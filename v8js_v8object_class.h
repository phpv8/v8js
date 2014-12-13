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

#ifndef V8JS_V8OBJECT_CLASS_H
#define V8JS_V8OBJECT_CLASS_H

/* {{{ Object container */
struct v8js_v8object {
	zend_object std;
	v8::Persistent<v8::Value> v8obj;
	int flags;
	struct v8js_ctx *ctx;
	HashTable *properties;
};
/* }}} */

extern zend_class_entry *php_ce_v8object;
extern zend_class_entry *php_ce_v8function;

/* Create PHP V8 object */
void v8js_v8object_create(zval *, v8::Handle<v8::Value>, int, v8::Isolate * TSRMLS_DC);

PHP_MINIT_FUNCTION(v8js_v8object_class);

#endif /* V8JS_V8OBJECT_CLASS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
