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

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
}

#include "php_v8js_macros.h"
#include "v8js_object_export.h"
#include "v8js_v8object_class.h"
#include "v8js_v8.h"

#include <stdexcept>
#include <limits>

static int v8js_is_assoc_array(HashTable *myht TSRMLS_DC) /* {{{ */
{
	int i;
	char *key;
	ulong index, idx = 0;
	uint key_len;
	HashPosition pos;

	zend_hash_internal_pointer_reset_ex(myht, &pos);
	for (;; zend_hash_move_forward_ex(myht, &pos)) {
		i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
		if (i == HASH_KEY_NON_EXISTANT)
			break;
		if (i == HASH_KEY_IS_STRING || index != idx) {
			return 1;
		}
		idx++;
	}
	return 0;
}
/* }}} */


static v8::Handle<v8::Value> v8js_hash_to_jsarr(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	HashTable *myht = HASH_OF(value);
	int i = myht ? zend_hash_num_elements(myht) : 0;

	/* Return object if dealing with assoc array */
	if (i > 0 && v8js_is_assoc_array(myht TSRMLS_CC)) {
		return v8js_hash_to_jsobj(value, isolate TSRMLS_CC);
	}

	v8::Local<v8::Array> newarr;

	/* Prevent recursion */
	if (myht && myht->nApplyCount > 1) {
		return V8JS_NULL;
	}

	newarr = v8::Array::New(isolate, i);

	if (i > 0)
	{
		zval **data;
		ulong index = 0;
		HashTable *tmp_ht;
		HashPosition pos;

		for (zend_hash_internal_pointer_reset_ex(myht, &pos);
			SUCCESS == zend_hash_get_current_data_ex(myht, (void **) &data, &pos);
			zend_hash_move_forward_ex(myht, &pos)
		) {
			tmp_ht = HASH_OF(*data);

			if (tmp_ht) {
				tmp_ht->nApplyCount++;
			}

			newarr->Set(index++, zval_to_v8js(*data, isolate TSRMLS_CC));

			if (tmp_ht) {
				tmp_ht->nApplyCount--;
			}
		}
	}
	return newarr;
}
/* }}} */

v8::Handle<v8::Value> zval_to_v8js(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	v8::Handle<v8::Value> jsValue;
	long v;
	zend_class_entry *ce;

	switch (Z_TYPE_P(value))
	{
		case IS_ARRAY:
			jsValue = v8js_hash_to_jsarr(value, isolate TSRMLS_CC);
			break;

		case IS_OBJECT:
             if (V8JSG(use_date)) {
				 ce = php_date_get_date_ce();
				 if (instanceof_function(Z_OBJCE_P(value), ce TSRMLS_CC)) {
					 zval *dtval;
					 zend_call_method_with_0_params(&value, NULL, NULL, "getTimestamp", &dtval);
					 if (dtval) {
						 jsValue = V8JS_DATE(((double)Z_LVAL_P(dtval) * 1000.0));
						 zval_ptr_dtor(&dtval);
					 }
					 else
						 jsValue = V8JS_NULL;
				 } else
					 jsValue = v8js_hash_to_jsobj(value, isolate TSRMLS_CC);
			 } else
				 jsValue = v8js_hash_to_jsobj(value, isolate TSRMLS_CC);
			break;

		case IS_STRING:
			jsValue = V8JS_STRL(Z_STRVAL_P(value), Z_STRLEN_P(value));
			break;

		case IS_LONG:
		    v = Z_LVAL_P(value);
			/* On Windows there are max and min macros, which would clobber the
			 * method names of std::numeric_limits< > otherwise. */
#undef max
#undef min
			if (v < - std::numeric_limits<int32_t>::min() || v > std::numeric_limits<int32_t>::max()) {
				jsValue = V8JS_FLOAT((double)v);
			} else {
				jsValue = V8JS_INT(v);
			}
			break;

		case IS_DOUBLE:
			jsValue = V8JS_FLOAT(Z_DVAL_P(value));
			break;

		case IS_BOOL:
			jsValue = V8JS_BOOL(Z_BVAL_P(value));
			break;

		default:
		case IS_NULL:
			jsValue = V8JS_NULL;
			break;
	}
	return jsValue;
}
/* }}} */

int v8js_to_zval(v8::Handle<v8::Value> jsValue, zval *return_value, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	if (jsValue->IsString())
	{
		v8::String::Utf8Value str(jsValue);
		const char *cstr = ToCString(str);
		RETVAL_STRINGL(cstr, jsValue->ToString()->Utf8Length(), 1);
//		RETVAL_STRING(cstr, 1);
	}
	else if (jsValue->IsBoolean())
	{
		RETVAL_BOOL(jsValue->Uint32Value());
	}
	else if (jsValue->IsInt32() || jsValue->IsUint32())
	{
		RETVAL_LONG((long) jsValue->IntegerValue());
	}
	else if (jsValue->IsNumber())
	{
		RETVAL_DOUBLE(jsValue->NumberValue());
	}
	else if (jsValue->IsDate())	/* Return as a PHP DateTime object */
	{
		v8::String::Utf8Value str(jsValue);
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
#if PHP_VERSION_ID < 50304
		zval *param;

		MAKE_STD_ZVAL(param);
		ZVAL_STRING(param, date_str, 0);

		object_init_ex(return_value, ce TSRMLS_CC);
		zend_call_method_with_1_params(&return_value, ce, &ce->constructor, "__construct", NULL, param);
		zval_ptr_dtor(&param);

		if (EG(exception)) {
			return FAILURE;
		}
#else
		php_date_instantiate(ce, return_value TSRMLS_CC);
		if (!php_date_initialize((php_date_obj *) zend_object_store_get_object(return_value TSRMLS_CC), date_str, strlen(date_str), NULL, NULL, 0 TSRMLS_CC)) {
			efree(date_str);
			return FAILURE;
		}

		efree(date_str);
#endif
	}
	else if (jsValue->IsObject())
	{
		v8::Handle<v8::Object> self = v8::Handle<v8::Object>::Cast(jsValue);
		// if this is a wrapped PHP object, then just unwrap it.
		v8::Local<v8::Value> php_object = self->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY));
		if (!php_object.IsEmpty()) {
			zval *object = reinterpret_cast<zval *>(v8::External::Cast(*php_object)->Value());
			RETVAL_ZVAL(object, 1, 0);
			return SUCCESS;
		}
		if ((flags & V8JS_FLAG_FORCE_ARRAY) || jsValue->IsArray()) {
			array_init(return_value);
			return v8js_get_properties_hash(jsValue, Z_ARRVAL_P(return_value), flags, isolate TSRMLS_CC);
		} else {
			v8js_v8object_create(return_value, jsValue, flags, isolate TSRMLS_CC);
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
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
