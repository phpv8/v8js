/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(_WIN32) && PHP_V8_API_VERSION >= 3029036
#include <libplatform/libplatform.h>
#endif

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
}

#include "php_v8js_macros.h"
#include "v8js_v8.h"
#include "v8js_debug.h"
#include "v8js_timer.h"
#include "v8js_exceptions.h"

void v8js_v8_init(TSRMLS_D) /* {{{ */
{
	/* Run only once */
	if (V8JSG(v8_initialized)) {
		return;
	}

#if !defined(_WIN32) && PHP_V8_API_VERSION >= 3029036
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
#endif

	/* Set V8 command line flags (must be done before V8::Initialize()!) */
	if (V8JSG(v8_flags)) {
		v8::V8::SetFlagsFromString(V8JSG(v8_flags), strlen(V8JSG(v8_flags)));
	}

	/* Initialize V8 */
	v8::V8::Initialize();

	/* Run only once */
	V8JSG(v8_initialized) = 1;
}
/* }}} */


void v8js_v8_call(v8js_ctx *c, zval **return_value,
				  long flags, long time_limit, long memory_limit,
				  std::function< v8::Local<v8::Value>(v8::Isolate *) >& v8_call TSRMLS_DC) /* {{{ */
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
	V8JS_GLOBAL_SET_FLAGS(isolate, flags);

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
			V8JSG(timer_thread) = new std::thread(v8js_timer_thread TSRMLS_CC);
		}
	}

	/* Always pass the timer to the stack so there can be follow-up changes to
	 * the time & memory limit. */
	v8js_timer_push(time_limit, memory_limit, c TSRMLS_CC);

#ifdef ENABLE_DEBUGGER_SUPPORT
	if(c == v8js_debug_context && v8js_debug_auto_break_mode != V8JS_DEBUG_AUTO_BREAK_NEVER) {
		v8::Debug::DebugBreak(c->isolate);

		if(v8js_debug_auto_break_mode == V8JS_DEBUG_AUTO_BREAK_ONCE) {
			/* If break-once-mode was enabled, reset flag. */
			v8js_debug_auto_break_mode = V8JS_DEBUG_AUTO_BREAK_NEVER;
		}
	}
#endif  /* ENABLE_DEBUGGER_SUPPORT */

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

	/* Check for fatal error marker possibly set by v8js_error_handler; just
	 * rethrow the error since we're now out of V8. */
	if(V8JSG(fatal_error_abort)) {
		zend_bailout();
	}

	char exception_string[64];

	if (c->time_limit_hit) {
		// Execution has been terminated due to time limit
		sprintf(exception_string, "Script time limit of %lu milliseconds exceeded", time_limit);
		zend_throw_exception(php_ce_v8js_time_limit_exception, exception_string, 0 TSRMLS_CC);
		return;
	}

	if (c->memory_limit_hit) {
		// Execution has been terminated due to memory limit
		sprintf(exception_string, "Script memory limit of %lu bytes exceeded", memory_limit);
		zend_throw_exception(php_ce_v8js_memory_limit_exception, exception_string, 0 TSRMLS_CC);
		return;
	}

	if (!try_catch.CanContinue()) {
		// At this point we can't re-throw the exception
		return;
	}

	/* There was pending exception left from earlier executions -> throw to PHP */
	if (c->pending_exception) {
		zend_throw_exception_object(c->pending_exception TSRMLS_CC);
		c->pending_exception = NULL;
	}

	/* Handle runtime JS exceptions */
	if (try_catch.HasCaught()) {

		/* Pending exceptions are set only in outer caller, inner caller exceptions are always rethrown */
		if (c->in_execution < 1) {

			/* Report immediately if report_uncaught is true */
			if (c->report_uncaught) {
				v8js_throw_script_exception(&try_catch TSRMLS_CC);
				return;
			}

			/* Exception thrown from JS, preserve it for future execution */
			if (result.IsEmpty()) {
				MAKE_STD_ZVAL(c->pending_exception);
				v8js_create_script_exception(c->pending_exception, &try_catch TSRMLS_CC);
				return;
			}
		}

		/* Rethrow back to JS */
		try_catch.ReThrow();
		return;
	}

	/* Convert V8 value to PHP value */
	if (!result.IsEmpty()) {
		v8js_to_zval(result, *return_value, flags, c->isolate TSRMLS_CC);
	}
}
/* }}} */

void v8js_terminate_execution(v8js_ctx *c TSRMLS_DC) /* {{{ */
{
	// Forcefully terminate the current thread of V8 execution in the isolate
	v8::V8::TerminateExecution(c->isolate);
}
/* }}} */


int v8js_get_properties_hash(v8::Handle<v8::Value> jsValue, HashTable *retval, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
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
			const char *key = ToCString(cstr);
			zval *value = NULL;

			v8::Local<v8::Value> php_object;
			if (jsVal->IsObject()) {
				php_object = v8::Local<v8::Object>::Cast(jsVal)->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY));
			}
			if (!php_object.IsEmpty()) {
				/* This is a PHP object, passed to JS and back. */
				value = reinterpret_cast<zval *>(v8::External::Cast(*php_object)->Value());
				Z_ADDREF_P(value);
			}
			else {
				MAKE_STD_ZVAL(value);

				if (v8js_to_zval(jsVal, value, flags, isolate TSRMLS_CC) == FAILURE) {
					zval_ptr_dtor(&value);
					return FAILURE;
				}
			}

			if ((flags & V8JS_FLAG_FORCE_ARRAY) || jsValue->IsArray()) {
				zend_symtable_update(retval, key, strlen(key) + 1, (void *)&value, sizeof(zval *), NULL);
			} else {
				zend_hash_update(retval, key, strlen(key) + 1, (void *) &value, sizeof(zval *), NULL);
			}
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
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
