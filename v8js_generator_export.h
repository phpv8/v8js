/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2016 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifndef V8JS_GENERATOR_EXPORT_H
#define V8JS_GENERATOR_EXPORT_H

v8::Local<v8::Value> v8js_wrap_generator(v8::Isolate *isolate, v8::Local<v8::Value> wrapped_object);

#endif /* V8JS_GENERATOR_EXPORT_H */

/*
 * Local variables:
 * mode: c++
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
