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

#ifndef V8JS_OBJECT_EXPORT_H
#define V8JS_OBJECT_EXPORT_H

v8::Handle<v8::Value> php_v8js_hash_to_jsobj(zval *value, v8::Isolate *isolate TSRMLS_DC);

#endif /* V8JS_OBJECT_EXPORT_H */
