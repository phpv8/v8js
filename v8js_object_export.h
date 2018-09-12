/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  +----------------------------------------------------------------------+
*/

#ifndef V8JS_OBJECT_EXPORT_H
#define V8JS_OBJECT_EXPORT_H

v8::Local<v8::Value> v8js_hash_to_jsobj(zval *value, v8::Isolate *isolate);


typedef enum {
	V8JS_PROP_GETTER,
	V8JS_PROP_SETTER,
	V8JS_PROP_QUERY,
	V8JS_PROP_DELETER
} property_op_t;

template<typename T>
v8::Local<v8::Value> v8js_named_property_callback(v8::Local<v8::Name> property,
						      const v8::PropertyCallbackInfo<T> &info,
						      property_op_t callback_type,
						      v8::Local<v8::Value> set_value = v8::Local<v8::Value>());

void v8js_php_callback(const v8::FunctionCallbackInfo<v8::Value>& info);

#endif /* V8JS_OBJECT_EXPORT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
