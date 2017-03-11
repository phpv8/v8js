/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"
#include "v8js_v8.h"
#include "v8js_timer.h"
#include "v8js_exceptions.h"

extern "C" {
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
}

#include <libplatform/libplatform.h>


void v8js_v8_init() /* {{{ */
{
	/* Run only once; thread-local test first */
	if (V8JSG(v8_initialized)) {
		return;
	}

	/* Set thread-local flag, that V8 was initialized. */
	V8JSG(v8_initialized) = 1;

#ifdef ZTS
	v8js_process_globals.lock.lock();

	if(v8js_process_globals.v8_initialized) {
		/* V8 already has been initialized by another thread */
		v8js_process_globals.lock.unlock();
		return;
	}
#endif

#if defined(PHP_V8_NATIVES_BLOB_PATH) && defined(PHP_V8_SNAPSHOT_BLOB_PATH)
	/* V8 doesn't work without startup data, load it. */
	v8::V8::InitializeExternalStartupData(
		PHP_V8_NATIVES_BLOB_PATH,
		PHP_V8_SNAPSHOT_BLOB_PATH
	);
#endif

	v8js_process_globals.v8_platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(v8js_process_globals.v8_platform);

	/* Set V8 command line flags (must be done before V8::Initialize()!) */
	if (v8js_process_globals.v8_flags) {
		size_t flags_len = strlen(v8js_process_globals.v8_flags);

		if (flags_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Length of V8 flags exceeds maximum supported length", 0);
		}
		else {
			v8::V8::SetFlagsFromString(v8js_process_globals.v8_flags, static_cast<int>(flags_len));
		}
	}

	/* Initialize V8 */
	v8::V8::Initialize();

#ifdef ZTS
	v8js_process_globals.v8_initialized = 1;
	v8js_process_globals.lock.unlock();
#endif
}
/* }}} */


/**
 * Prepare V8 call trampoline with time & memory limit, exception handling, etc.
 *
 * The caller MUST check V8JSG(fatal_error_abort) and trigger further bailout
 * either immediately after this function returns (or possibly after freeing
 * heap allocated memory).
 */
void v8js_v8_call(v8js_ctx *c, zval **return_value,
				  long flags, long time_limit, size_t memory_limit,
				  std::function< v8::Local<v8::Value>(v8::Isolate *) >& v8_call) /* {{{ */
{
	char *tz = NULL;

	V8JS_CTX_PROLOGUE(c);

	V8JSG(timer_mutex).lock();
	c->time_limit_hit = false;
	c->memory_limit_hit = false;
	V8JSG(timer_mutex).unlock();

	/* Catch JS exceptions */
	v8::TryCatch try_catch;

	/* Set flags for runtime use */
	c->flags = flags;

	/* Check if timezone has been changed and notify V8 */
	tz = getenv("TZ");

	if (tz != NULL) {
		if (c->tz == NULL) {
			c->tz = strdup(tz);
		}
		else if (strcmp(c->tz, tz) != 0) {
			v8::Date::DateTimeConfigurationChangeNotification(c->isolate);

			free(c->tz);
			c->tz = strdup(tz);
		}
	}

	if (time_limit > 0 || memory_limit > 0) {
		// If timer thread is not running then start it
		if (!V8JSG(timer_thread)) {
			// If not, start timer thread
			V8JSG(timer_thread) = new std::thread(v8js_timer_thread, ZEND_MODULE_GLOBALS_BULK(v8js));
		}
	}

	/* Always pass the timer to the stack so there can be follow-up changes to
	 * the time & memory limit. */
	v8js_timer_push(time_limit, memory_limit, c);

	/* Execute script */
	c->in_execution++;
	v8::Local<v8::Value> result = v8_call(c->isolate);
	c->in_execution--;

	/* Pop our context from the stack and read (possibly updated) limits
	 * into local variables. */
	V8JSG(timer_mutex).lock();
	v8js_timer_ctx *timer_ctx = V8JSG(timer_stack).front();
	V8JSG(timer_stack).pop_front();
	V8JSG(timer_mutex).unlock();

	time_limit = timer_ctx->time_limit;
	memory_limit = timer_ctx->memory_limit;

	efree(timer_ctx);

	if(!V8JSG(fatal_error_abort)) {
		char exception_string[64];

		if (c->time_limit_hit) {
			// Execution has been terminated due to time limit
			sprintf(exception_string, "Script time limit of %lu milliseconds exceeded", time_limit);
			zend_throw_exception(php_ce_v8js_time_limit_exception, exception_string, 0);
			return;
		}

		if (memory_limit && !c->memory_limit_hit) {
			// Re-check memory limit (very short executions might never be hit by timer thread)
			v8::HeapStatistics hs;
			isolate->GetHeapStatistics(&hs);

			if (hs.used_heap_size() > memory_limit) {
				isolate->LowMemoryNotification();
				isolate->GetHeapStatistics(&hs);

				if (hs.used_heap_size() > memory_limit) {
					c->memory_limit_hit = true;
				}
			}
		}

		if (c->memory_limit_hit) {
			// Execution has been terminated due to memory limit
			sprintf(exception_string, "Script memory limit of %lu bytes exceeded", memory_limit);
			zend_throw_exception(php_ce_v8js_memory_limit_exception, exception_string, 0);
			return;
		}

		if (!try_catch.CanContinue()) {
			// At this point we can't re-throw the exception
			return;
		}

		/* There was pending exception left from earlier executions -> throw to PHP */
		if (Z_TYPE(c->pending_exception) == IS_OBJECT) {
			zend_throw_exception_object(&c->pending_exception);
			ZVAL_NULL(&c->pending_exception);
		}

		/* Handle runtime JS exceptions */
		if (try_catch.HasCaught()) {

			/* Pending exceptions are set only in outer caller, inner caller exceptions are always rethrown */
			if (c->in_execution < 1) {

				/* Report immediately if report_uncaught is true */
				if (c->report_uncaught) {
					v8js_throw_script_exception(c->isolate, &try_catch);
					return;
				}

				/* Exception thrown from JS, preserve it for future execution */
				if (result.IsEmpty()) {
					v8js_create_script_exception(&c->pending_exception, c->isolate, &try_catch);
					return;
				}
			}

			/* Rethrow back to JS */
			try_catch.ReThrow();
			return;
		}

		/* Convert V8 value to PHP value */
		if (return_value && !result.IsEmpty()) {
			v8js_to_zval(result, *return_value, flags, c->isolate);
		}
	}
}
/* }}} */

void v8js_terminate_execution(v8::Isolate *isolate) /* {{{ */
{
	if(v8::V8::IsExecutionTerminating(isolate)) {
		/* Execution already terminating, needn't trigger it again and
		 * especially must not execute the spinning loop (which would cause
		 * crashes in V8 itself, at least with 4.2 and 4.3 version lines). */
		return;
	}

	/* Unfortunately just calling TerminateExecution on the isolate is not
	 * enough, since v8 just marks the thread as "to be aborted" and doesn't
	 * immediately do so.  Hence we enter an endless loop after signalling
	 * termination, so we definitely don't execute JS code after the exit()
	 * statement. */
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);

	v8::Local<v8::String> source = V8JS_STR("for(;;);");
	v8::Local<v8::Script> script = v8::Script::Compile(source);
	v8::V8::TerminateExecution(isolate);
	script->Run();
}
/* }}} */


int v8js_get_properties_hash(v8::Local<v8::Value> jsValue, HashTable *retval, int flags, v8::Isolate *isolate) /* {{{ */
{
	v8::Local<v8::Object> jsObj = jsValue->ToObject();

	if (!jsObj.IsEmpty()) {
		v8::Local<v8::Array> jsKeys = jsObj->GetPropertyNames();

		for (unsigned i = 0; i < jsKeys->Length(); i++)
		{
			v8::Local<v8::String> jsKey = jsKeys->Get(i)->ToString();

			/* Skip any prototype properties */
			if (!jsObj->HasOwnProperty(jsKey) && !jsObj->HasRealNamedProperty(jsKey) && !jsObj->HasRealNamedCallbackProperty(jsKey)) {
				continue;
			}

			v8::Local<v8::Value> jsVal = jsObj->Get(jsKey);
			v8::String::Utf8Value cstr(jsKey);
			const char *c_key = ToCString(cstr);
			zend_string *key = zend_string_init(c_key, jsKey->ToString()->Utf8Length(), 0);
			zval value;
			ZVAL_UNDEF(&value);

			if (jsVal->IsObject() && jsVal->ToObject()->InternalFieldCount() == 2) {
				/* This is a PHP object, passed to JS and back. */
				zend_object *object = reinterpret_cast<zend_object *>(jsVal->ToObject()->GetAlignedPointerFromInternalField(1));
				ZVAL_OBJ(&value, object);
				Z_ADDREF_P(&value);
			}
			else {
				if (v8js_to_zval(jsVal, &value, flags, isolate) == FAILURE) {
					zval_ptr_dtor(&value);
					return FAILURE;
				}
			}

			if ((flags & V8JS_FLAG_FORCE_ARRAY) || jsValue->IsArray()) {
				zend_symtable_update(retval, key, &value);
			} else {
				zend_hash_update(retval, key, &value);
			}

			zend_string_release(key);
		}
		return SUCCESS;
	}
	return FAILURE;
}
/* }}} */




/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
