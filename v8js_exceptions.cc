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

extern "C" {
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}

/* {{{ Class Entries */
zend_class_entry *php_ce_v8js_exception;
zend_class_entry *php_ce_v8js_script_exception;
zend_class_entry *php_ce_v8js_time_limit_exception;
zend_class_entry *php_ce_v8js_memory_limit_exception;
/* }}} */


/* {{{ Class: V8JsScriptException */

void v8js_create_script_exception(zval *return_value, v8::Isolate *isolate, v8::TryCatch *try_catch) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, ctx->context);

	v8::String::Utf8Value exception(isolate, try_catch->Exception());
	const char *exception_string = ToCString(exception);
	v8::Local<v8::Message> tc_message = try_catch->Message();
	const char *filename_string, *sourceline_string;
	char *message_string;

	object_init_ex(return_value, php_ce_v8js_script_exception);

#define PHPV8_EXPROP(type, name, value) \
	zend_update_property##type(php_ce_v8js_script_exception, return_value, #name, sizeof(#name) - 1, value);

	if (tc_message.IsEmpty()) {
		spprintf(&message_string, 0, "%s", exception_string);
	}
	else
	{
		v8::String::Utf8Value filename(isolate, tc_message->GetScriptResourceName());
		filename_string = ToCString(filename);
		PHPV8_EXPROP(_string, JsFileName, filename_string);

		v8::MaybeLocal<v8::String> maybe_sourceline = tc_message->GetSourceLine(context);
		if (!maybe_sourceline.IsEmpty()) {
			v8::String::Utf8Value sourceline(isolate, maybe_sourceline.ToLocalChecked());
			sourceline_string = ToCString(sourceline);
			PHPV8_EXPROP(_string, JsSourceLine, sourceline_string);
		}

		v8::Maybe<int> linenum = tc_message->GetLineNumber(context);
		if (linenum.IsJust()) {
			PHPV8_EXPROP(_long, JsLineNumber, linenum.FromJust());
		}

		v8::Maybe<int> start_col = tc_message->GetStartColumn(context);
		if (start_col.IsJust()) {
			PHPV8_EXPROP(_long, JsStartColumn, start_col.FromJust());
		}

		v8::Maybe<int> end_col = tc_message->GetEndColumn(context);
		if (end_col.IsJust()) {
			PHPV8_EXPROP(_long, JsEndColumn, end_col.FromJust());
		}

		spprintf(&message_string, 0, "%s:%d: %s", filename_string, linenum.FromMaybe(0), exception_string);

		v8::MaybeLocal<v8::Value> maybe_stacktrace = try_catch->StackTrace(context);
		if (!maybe_stacktrace.IsEmpty()) {
			v8::String::Utf8Value stacktrace(isolate, maybe_stacktrace.ToLocalChecked());
			PHPV8_EXPROP(_string, JsTrace, ToCString(stacktrace));
		}

		v8::Local<v8::Object> error_object;
		if(try_catch->Exception()->IsObject() && try_catch->Exception()->ToObject(context).ToLocal(&error_object) && error_object->InternalFieldCount() == 2) {
			zend_object *php_exception = reinterpret_cast<zend_object *>(error_object->GetAlignedPointerFromInternalField(1));

			zend_class_entry *exception_ce = zend_exception_get_default();
			if (instanceof_function(php_exception->ce, exception_ce)) {
#ifdef GC_ADDREF
				GC_ADDREF(php_exception);
#else
				++GC_REFCOUNT(php_exception);
#endif
				zend_exception_set_previous(Z_OBJ_P(return_value), php_exception);
			}
		}
	}

	PHPV8_EXPROP(_string, message, message_string);

	efree(message_string);
}
/* }}} */

void v8js_throw_script_exception(v8::Isolate *isolate, v8::TryCatch *try_catch) /* {{{ */
{
	v8::String::Utf8Value exception(isolate, try_catch->Exception());
	const char *exception_string = ToCString(exception);
	zval zexception;

	if (try_catch->Message().IsEmpty()) {
		zend_throw_exception(php_ce_v8js_script_exception, (char *) exception_string, 0);
	} else {
		v8js_create_script_exception(&zexception, isolate, try_catch);
		zend_throw_exception_object(&zexception);
	}
}
/* }}} */

#define V8JS_EXCEPTION_METHOD(property) \
	static PHP_METHOD(V8JsScriptException, get##property) \
	{ \
		zval *value, rv;							\
		\
		if (zend_parse_parameters_none() == FAILURE) { \
			return; \
		} \
		value = zend_read_property(php_ce_v8js_script_exception, getThis(), #property, sizeof(#property) - 1, 0, &rv); \
		RETURN_ZVAL(value, 1, 0); \
	}

/* {{{ proto string V8JsEScriptxception::getJsFileName()
 */
V8JS_EXCEPTION_METHOD(JsFileName);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsLineNumber()
 */
V8JS_EXCEPTION_METHOD(JsLineNumber);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsStartColumn()
 */
V8JS_EXCEPTION_METHOD(JsStartColumn);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsEndColumn()
 */
V8JS_EXCEPTION_METHOD(JsEndColumn);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsSourceLine()
 */
V8JS_EXCEPTION_METHOD(JsSourceLine);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsTrace()
 */
V8JS_EXCEPTION_METHOD(JsTrace);	
/* }}} */


ZEND_BEGIN_ARG_INFO(arginfo_v8jsscriptexception_no_args, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry v8js_script_exception_methods[] = { /* {{{ */
	PHP_ME(V8JsScriptException,	getJsFileName,		arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsLineNumber,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsStartColumn,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsEndColumn,		arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsSourceLine,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsTrace,			arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsScriptException */



/* {{{ Class: V8JsException */

static const zend_function_entry v8js_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsException */



/* {{{ Class: V8JsTimeLimitException */

static const zend_function_entry v8js_time_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsTimeLimitException */



/* {{{ Class: V8JsMemoryLimitException */

static const zend_function_entry v8js_memory_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsMemoryLimitException */



PHP_MINIT_FUNCTION(v8js_exceptions) /* {{{ */
{
	zend_class_entry ce;

	/* V8JsException Class */
	INIT_CLASS_ENTRY(ce, "V8JsException", v8js_exception_methods);
	php_ce_v8js_exception = zend_register_internal_class_ex(&ce, spl_ce_RuntimeException);

	/* V8JsScriptException Class */
	INIT_CLASS_ENTRY(ce, "V8JsScriptException", v8js_script_exception_methods);
	php_ce_v8js_script_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception);
	php_ce_v8js_script_exception->ce_flags |= ZEND_ACC_FINAL;

	/* Add custom JS specific properties */
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsFileName"),		ZEND_ACC_PROTECTED);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsLineNumber"),		ZEND_ACC_PROTECTED);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsStartColumn"),	ZEND_ACC_PROTECTED);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsEndColumn"),		ZEND_ACC_PROTECTED);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsSourceLine"),		ZEND_ACC_PROTECTED);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsTrace"),			ZEND_ACC_PROTECTED);

	/* V8JsTimeLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsTimeLimitException", v8js_time_limit_exception_methods);
	php_ce_v8js_time_limit_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception);
	php_ce_v8js_time_limit_exception->ce_flags |= ZEND_ACC_FINAL;

	/* V8JsMemoryLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsMemoryLimitException", v8js_memory_limit_exception_methods);
	php_ce_v8js_memory_limit_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception);
	php_ce_v8js_memory_limit_exception->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
