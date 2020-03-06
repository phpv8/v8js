/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
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

#include <stdexcept>
#include <limits>

#include "php_v8js_macros.h"
#include "v8js_exceptions.h"
#include "v8js_object_export.h"
#include "v8js_v8object_class.h"
#include "v8js_v8.h"

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
}

static int v8js_is_assoc_array(HashTable *myht) /* {{{ */
{
	zend_string *key;
	zend_ulong index, idx = 0;

	ZEND_HASH_FOREACH_KEY(myht, index, key) {
		if(key) {
			// HASH_KEY_IS_STRING
			return 1;
		}

		if(index != idx) {
			return 1;
		}

		idx ++;
	} ZEND_HASH_FOREACH_END();

	return 0;
}
/* }}} */


static v8::Local<v8::Value> v8js_hash_to_jsarr(zval *value, v8::Isolate *isolate) /* {{{ */
{
	HashTable *myht = HASH_OF(value);
	int i = myht ? zend_hash_num_elements(myht) : 0;

	/* Return object if dealing with assoc array */
	if (i > 0 && v8js_is_assoc_array(myht)) {
		return v8js_hash_to_jsobj(value, isolate);
	}

	v8::Local<v8::Array> newarr;

	/* Prevent recursion */
#if PHP_VERSION_ID >= 70300
	if (myht && GC_IS_RECURSIVE(myht))
#else
	if (myht && ZEND_HASH_GET_APPLY_COUNT(myht) > 0)
#endif
	{
		return V8JS_NULL;
	}

	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();
	newarr = v8::Array::New(isolate, i);

	if (i > 0)
	{
		zval *data;
		ulong index = 0;

#if PHP_VERSION_ID >= 70300
		if (myht && !(GC_FLAGS(myht) & GC_IMMUTABLE))
#else
		if (myht)
#endif
		{
			GC_PROTECT_RECURSION(myht);
		}

		ZEND_HASH_FOREACH_VAL(myht, data) {
			newarr->Set(v8_context, index++, zval_to_v8js(data, isolate));
		} ZEND_HASH_FOREACH_END();

#if PHP_VERSION_ID >= 70300
		if (myht && !(GC_FLAGS(myht) & GC_IMMUTABLE))
#else
		if (myht)
#endif
		{
			GC_UNPROTECT_RECURSION(myht);
		}
	}
	return newarr;
}
/* }}} */

v8::Local<v8::Value> zend_long_to_v8js(zend_long v, v8::Isolate *isolate) /* {{{ */
{
	if (v < std::numeric_limits<int32_t>::min() || v > std::numeric_limits<int32_t>::max()) {
		return V8JS_FLOAT(static_cast<double>(v));
	} else {
		return V8JS_INT(static_cast<int32_t>(v));
	}
}
/* }}} */

v8::Local<v8::Value> zval_to_v8js(zval *value, v8::Isolate *isolate) /* {{{ */
{
	v8::Local<v8::Value> jsValue;
	zend_string *value_str;
	zend_class_entry *ce;

	switch (Z_TYPE_P(value))
	{
		case IS_INDIRECT:
			jsValue = zval_to_v8js(Z_INDIRECT_P(value), isolate);
			break;

		case IS_REFERENCE:
			jsValue = zval_to_v8js(Z_REFVAL_P(value), isolate);
			break;

		case IS_ARRAY:
			jsValue = v8js_hash_to_jsarr(value, isolate);
			break;

		case IS_OBJECT:
             if (V8JSG(use_date)) {
				 ce = php_date_get_date_ce();
				 if (instanceof_function(Z_OBJCE_P(value), ce)) {
					 zval dtval;
					 zend_call_method_with_0_params(value, NULL, NULL, "getTimestamp", &dtval);
					 v8::Date::New(isolate->GetEnteredContext(), ((double)Z_LVAL(dtval) * 1000.0)).ToLocal(&jsValue);
					 zval_dtor(&dtval);
				 } else
					 jsValue = v8js_hash_to_jsobj(value, isolate);
			 } else
				 jsValue = v8js_hash_to_jsobj(value, isolate);
			break;

		case IS_STRING:
			value_str = Z_STR_P(value);
			if (ZSTR_LEN(value_str) > std::numeric_limits<int>::max()) {
				zend_throw_exception(php_ce_v8js_exception,
					"String exceeds maximum string length", 0);
				break;
			}

			jsValue = V8JS_ZSTR(value_str);
			break;

		case IS_LONG:
			jsValue = zend_long_to_v8js(Z_LVAL_P(value), isolate);
			break;

		case IS_DOUBLE:
			jsValue = V8JS_FLOAT(Z_DVAL_P(value));
			break;

		case IS_TRUE:
			jsValue = V8JS_TRUE();
			break;

		case IS_FALSE:
			jsValue = V8JS_FALSE();
			break;

		case IS_NULL:
			jsValue = V8JS_NULL;
			break;

		case IS_UNDEF:
		default:
			/* undefined -> return v8::Value left empty */
			jsValue = v8::Undefined(isolate);
			break;
	}

	return jsValue;
}
/* }}} */

int v8js_to_zval(v8::Local<v8::Value> jsValue, zval *return_value, int flags, v8::Isolate *isolate) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, ctx->context);

	if (jsValue->IsString())
	{
		v8::String::Utf8Value str(isolate, jsValue);
		const char *cstr = ToCString(str);
		RETVAL_STRINGL(cstr, str.length());
	}
	else if (jsValue->IsBoolean())
	{
		bool value = jsValue->BooleanValue(isolate);
		RETVAL_BOOL(value);
	}
	else if (jsValue->IsInt32() || jsValue->IsUint32())
	{
		v8::Maybe<int64_t> value = jsValue->IntegerValue(v8_context);
		if (value.IsNothing())
		{
			return FAILURE;
		}
		RETVAL_LONG((long) value.ToChecked());
	}
	else if (jsValue->IsNumber())
	{
		v8::Maybe<double> value = jsValue->NumberValue(v8_context);
		if (value.IsNothing())
		{
			return FAILURE;
		}
		RETVAL_DOUBLE(value.ToChecked());
	}
	else if (jsValue->IsDate())	/* Return as a PHP DateTime object */
	{
		v8::String::Utf8Value str(isolate, jsValue);
		const char *cstr = ToCString(str);

		/* cstr has two timezone specifications:
		 *
		 * example from Linux:
		 * Mon Sep 08 1975 09:00:00 GMT+0000 (UTC)
		 *
		 * example from Windows:
		 * Mon Sep 08 1975 11:00:00 GMT+0200 (W. Europe Daylight Time)
		 *
		 * ... problem is, that PHP can't parse the second timezone
		 * specification as returned by v8 running on Windows.  And as a
		 * matter of that fails due to inconsistent second timezone spec
		 */
		char *date_str = estrdup(cstr);
		char *paren_ptr = strchr(date_str, '(');

		if (paren_ptr != NULL) {
			*paren_ptr = 0;
		}

		zend_class_entry *ce = php_date_get_date_ce();
		php_date_instantiate(ce, return_value);
		if (!php_date_initialize(Z_PHPDATE_P(return_value), date_str, strlen(date_str), NULL, NULL, 0)) {
			efree(date_str);
			return FAILURE;
		}

		efree(date_str);
	}
	else if (jsValue->IsObject())
	{
		v8::Local<v8::Object> self;
		if (!jsValue->ToObject(v8_context).ToLocal(&self))
		{
			return FAILURE;
		}

		// if this is a wrapped PHP object, then just unwrap it.
		if (self->InternalFieldCount() == 2) {
			zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));
			zval zval_object;
			ZVAL_OBJ(&zval_object, object);
			RETVAL_ZVAL(&zval_object, 1, 0);
			return SUCCESS;
		}

		if ((flags & V8JS_FLAG_FORCE_ARRAY && !jsValue->IsFunction()) || jsValue->IsArray()) {
			array_init(return_value);
			return v8js_get_properties_hash(jsValue, Z_ARRVAL_P(return_value), flags, isolate);
		} else {
			v8js_v8object_create(return_value, jsValue, flags, isolate);
			return SUCCESS;
		}
	}
	else /* types External, RegExp, Undefined and Null are considered NULL */
	{
		RETVAL_NULL();
	}

 	return SUCCESS;
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
