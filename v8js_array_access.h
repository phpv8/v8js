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

#ifndef V8JS_ARRAY_ACCESS_H
#define V8JS_ARRAY_ACCESS_H

v8::Handle<v8::Value> php_v8js_array_access_to_jsobj(zval *value, v8::Isolate *isolate TSRMLS_DC);

#endif /* V8JS_ARRAY_ACCESS_H */
