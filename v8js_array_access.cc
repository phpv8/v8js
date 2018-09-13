/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"
#include "v8js_array_access.h"
#include "v8js_exceptions.h"
#include "v8js_object_export.h"

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
}

static zval v8js_array_access_dispatch(zend_object *object, const char *method_name, int param_count,
									   uint32_t index, zval zvalue) /* {{{ */
{
	zend_fcall_info fci;
	zval php_value;

	fci.size = sizeof(fci);
#if (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
	fci.function_table = &object->ce->function_table;
	fci.symbol_table = NULL;
#endif
	ZVAL_STRING(&fci.function_name, method_name);
	fci.retval = &php_value;

	zval params[2];
	ZVAL_LONG(&params[0], index);
	params[1] = zvalue;

	fci.param_count = param_count;
	fci.params = params;

	fci.object = object;
	fci.no_separation = 0;

	zend_call_function(&fci, NULL);
	zval_dtor(&fci.function_name);
	return php_value;
}
/* }}} */



void v8js_array_access_getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	zval zvalue;
	ZVAL_UNDEF(&zvalue);

	zval php_value = v8js_array_access_dispatch(object, "offsetGet", 1, index, zvalue);
	v8::Local<v8::Value> ret_value = zval_to_v8js(&php_value, isolate);
	zval_ptr_dtor(&php_value);

	info.GetReturnValue().Set(ret_value);
}
/* }}} */

void v8js_array_access_setter(uint32_t index, v8::Local<v8::Value> value,
								  const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	zval zvalue;
	ZVAL_UNDEF(&zvalue);

	if (v8js_to_zval(value, &zvalue, 0, isolate) != SUCCESS) {
		info.GetReturnValue().Set(v8::Local<v8::Value>());
		return;
	}

	zval php_value = v8js_array_access_dispatch(object, "offsetSet", 2, index, zvalue);
	zval_ptr_dtor(&php_value);

	/* simply pass back the value to tell we intercepted the call
	 * as the offsetSet function returns void. */
	info.GetReturnValue().Set(value);

	/* if PHP wanted to hold on to this value, zend_call_function would
	 * have bumped the refcount. */
	zval_ptr_dtor(&zvalue);
}
/* }}} */


static int v8js_array_access_get_count_result(zend_object *object) /* {{{ */
{
	zval zvalue;
	ZVAL_UNDEF(&zvalue);

	zval php_value = v8js_array_access_dispatch(object, "count", 0, 0, zvalue);

	if(Z_TYPE(php_value) != IS_LONG) {
		php_error_docref(NULL, E_WARNING, "Non-numeric return value from count() method");
		zval_ptr_dtor(&php_value);
		return 0;
	}

	zend_long result = Z_LVAL(php_value);
	
	if (result > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Array size/offset exceeds maximum supported length", 0);
		return 0;
	}

	return static_cast<int>(result);
}
/* }}} */

static bool v8js_array_access_isset_p(zend_object *object, int index) /* {{{ */
{
	zval zvalue;
	ZVAL_UNDEF(&zvalue);

	zval php_value = v8js_array_access_dispatch(object, "offsetExists", 1, index, zvalue);

	if(Z_TYPE(php_value) != IS_TRUE && Z_TYPE(php_value) != IS_FALSE) {
		php_error_docref(NULL, E_WARNING, "Non-boolean return value from offsetExists() method");
		zval_ptr_dtor(&php_value);
		return false;
	}

	return Z_TYPE(php_value) == IS_TRUE;
}
/* }}} */


static void v8js_array_access_length(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	int length = v8js_array_access_get_count_result(object);
	info.GetReturnValue().Set(V8JS_INT(length));
}
/* }}} */

void v8js_array_access_deleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	zval zvalue;
	ZVAL_UNDEF(&zvalue);

	zval php_value = v8js_array_access_dispatch(object, "offsetUnset", 1, index, zvalue);
	zval_ptr_dtor(&php_value);

	info.GetReturnValue().Set(V8JS_BOOL(true));
}
/* }}} */

void v8js_array_access_query(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	/* If index is set, then return an integer encoding a v8::PropertyAttribute;
	 * otherwise we're expected to return an empty handle. */
	if(v8js_array_access_isset_p(object, index)) {
		info.GetReturnValue().Set(V8JS_UINT(v8::PropertyAttribute::None));
	}
}
/* }}} */


void v8js_array_access_enumerator(const v8::PropertyCallbackInfo<v8::Array>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));

	int length = v8js_array_access_get_count_result(object);
	v8::Local<v8::Array> result = v8::Array::New(isolate, length);

	int i = 0;

	for(int j = 0; j < length; j ++) {
		if(v8js_array_access_isset_p(object, j)) {
			result->Set(isolate->GetEnteredContext(), i ++, V8JS_INT(j));
		}
	}

	result->Set(isolate->GetEnteredContext(), V8JS_SYM("length"), V8JS_INT(i));
	info.GetReturnValue().Set(result);
}
/* }}} */



void v8js_array_access_named_getter(v8::Local<v8::Name> property_name, const v8::PropertyCallbackInfo<v8::Value> &info) /* {{{ */
{
	v8::Local<v8::String> property = v8::Local<v8::String>::Cast(property_name);
	v8::Isolate *isolate = info.GetIsolate();
	v8::String::Utf8Value cstr(isolate, property);
	const char *name = ToCString(cstr);

	if(strcmp(name, "length") == 0) {
		v8js_array_access_length(property, info);
		return;
	}

	v8::Local<v8::Value> ret_value = v8js_named_property_callback(property, info, V8JS_PROP_GETTER);

	if(ret_value.IsEmpty()) {
		v8::Local<v8::Array> arr = v8::Array::New(isolate);
		v8::Local<v8::Value> prototype = arr->GetPrototype();

		if(!prototype->IsObject()) {
			/* ehh?  Array.prototype not an object? strange, stop. */
			info.GetReturnValue().Set(ret_value);
		}

		v8::Local<v8::Object> prototype_object;
		if(!prototype->ToObject(isolate->GetEnteredContext()).ToLocal(&prototype_object)) {
			/* ehh?  Array.prototype not an object? strange, stop. */
			info.GetReturnValue().Set(ret_value);
		}

		prototype_object->Get(isolate->GetEnteredContext(), property).ToLocal(&ret_value);
	}

	info.GetReturnValue().Set(ret_value);
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
