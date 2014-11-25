/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@brokenpipe.de>                          |
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
#include "v8js_array_access.h"

static void php_v8js_array_access_getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	V8JS_TSRMLS_FETCH();

	zval *object = reinterpret_cast<zval *>(self->GetAlignedPointerFromInternalField(0));
	zend_class_entry *ce = Z_OBJCE_P(object);

	/* Okay, let's call offsetGet. */
	zend_fcall_info fci;
	zval *php_value;

	zval fmember;
	INIT_ZVAL(fmember);
	ZVAL_STRING(&fmember, "offsetGet", 0);

	zval zindex;
	INIT_ZVAL(zindex);
	ZVAL_LONG(&zindex, index);

	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.function_name = &fmember;
	fci.symbol_table = NULL;
	fci.retval_ptr_ptr = &php_value;

	zval *zindex_ptr = &zindex;
	zval **zindex_ptr_ptr = &zindex_ptr;
	fci.param_count = 1;
	fci.params = &zindex_ptr_ptr;

	fci.object_ptr = object;
	fci.no_separation = 0;

	zend_call_function(&fci, NULL TSRMLS_CC);

	v8::Local<v8::Value> ret_value = zval_to_v8js(php_value, isolate TSRMLS_CC);
	zval_ptr_dtor(&php_value);

	info.GetReturnValue().Set(ret_value);
}
/* }}} */

static void php_v8js_array_access_setter(uint32_t index, v8::Local<v8::Value> value,
										 const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	V8JS_TSRMLS_FETCH();

	zval *object = reinterpret_cast<zval *>(self->GetAlignedPointerFromInternalField(0));
	zend_class_entry *ce = Z_OBJCE_P(object);

	/* Okay, let's call offsetSet. */
	zend_fcall_info fci;
	zval *php_value;

	zval fmember;
	INIT_ZVAL(fmember);
	ZVAL_STRING(&fmember, "offsetSet", 0);

	zval zindex;
	INIT_ZVAL(zindex);
	ZVAL_LONG(&zindex, index);

	zval *zvalue_ptr;
	MAKE_STD_ZVAL(zvalue_ptr);
	if (v8js_to_zval(value, zvalue_ptr, 0, isolate TSRMLS_CC) != SUCCESS) {
		info.GetReturnValue().Set(v8::Handle<v8::Value>());
		return;
	}

	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.function_name = &fmember;
	fci.symbol_table = NULL;
	fci.retval_ptr_ptr = &php_value;

	zval *zindex_ptr = &zindex;
	zval **params[2] = { &zindex_ptr, &zvalue_ptr };

	fci.param_count = 2;
	fci.params = params;

	fci.object_ptr = object;
	fci.no_separation = 0;

	zend_call_function(&fci, NULL TSRMLS_CC);
	zval_ptr_dtor(&php_value);

	/* simply pass back the value to tell we intercepted the call
	 * as the offsetSet function returns void. */
	info.GetReturnValue().Set(value);

	/* if PHP wanted to hold on to this value, zend_call_function would
	 * have bumped the refcount. */
	zval_ptr_dtor(&zvalue_ptr);
}
/* }}} */

static void php_v8js_array_access_length(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	V8JS_TSRMLS_FETCH();

	zval *object = reinterpret_cast<zval *>(self->GetAlignedPointerFromInternalField(0));
	zend_class_entry *ce = Z_OBJCE_P(object);

	zend_fcall_info fci;
	zval *php_value;

	zval fmember;
	INIT_ZVAL(fmember);
	ZVAL_STRING(&fmember, "count", 0);

	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.function_name = &fmember;
	fci.symbol_table = NULL;
	fci.retval_ptr_ptr = &php_value;

	fci.param_count = 0;
	fci.params = NULL;

	fci.object_ptr = object;
	fci.no_separation = 0;

	zend_call_function(&fci, NULL TSRMLS_CC);

	v8::Local<v8::Value> ret_value = zval_to_v8js(php_value, isolate TSRMLS_CC);
	zval_ptr_dtor(&php_value);

	info.GetReturnValue().Set(ret_value);
}
/* }}} */


v8::Handle<v8::Value> php_v8js_array_access_to_jsobj(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	v8::Local<v8::ObjectTemplate> inst_tpl = v8::ObjectTemplate::New(isolate);
	inst_tpl->SetIndexedPropertyHandler(php_v8js_array_access_getter,
										php_v8js_array_access_setter);
	inst_tpl->SetAccessor(V8JS_STR("length"), php_v8js_array_access_length);
	inst_tpl->SetInternalFieldCount(1);

	v8::Handle<v8::Object> newobj = inst_tpl->NewInstance();
	newobj->SetAlignedPointerInInternalField(0, value);

	/* Change prototype of `newobj' to that of Array */
	v8::Local<v8::Array> arr = v8::Array::New(isolate);
	newobj->SetPrototype(arr->GetPrototype());

	return newobj;
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
