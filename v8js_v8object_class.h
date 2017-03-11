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

#ifndef V8JS_V8OBJECT_CLASS_H
#define V8JS_V8OBJECT_CLASS_H

/* {{{ Object container */
struct v8js_v8object {
	v8::Persistent<v8::Value> v8obj;
	int flags;
	struct v8js_ctx *ctx;
	HashTable *properties;
	zend_object std;
};
/* }}} */

extern zend_class_entry *php_ce_v8object;
extern zend_class_entry *php_ce_v8function;

/* Create PHP V8 object */
void v8js_v8object_create(zval *, v8::Local<v8::Value>, int, v8::Isolate *);

static inline v8js_v8object *v8js_v8object_fetch_object(zend_object *obj) {
	return (v8js_v8object *)((char *)obj - XtOffsetOf(struct v8js_v8object, std));
}

#define Z_V8JS_V8OBJECT_OBJ_P(zv) v8js_v8object_fetch_object(Z_OBJ_P(zv));


/* {{{ Generator container */
struct v8js_v8generator {
	zval value;
	bool primed;
	bool done;
	struct v8js_v8object v8obj;
};
/* }}} */

extern zend_class_entry *php_ce_v8generator;


static inline v8js_v8generator *v8js_v8generator_fetch_object(zend_object *obj) {
	return (v8js_v8generator *)((char *)obj - XtOffsetOf(struct v8js_v8generator, v8obj.std));
}

#define Z_V8JS_V8GENERATOR_OBJ_P(zv) v8js_v8generator_fetch_object(Z_OBJ_P(zv));



PHP_MINIT_FUNCTION(v8js_v8object_class);

#endif /* V8JS_V8OBJECT_CLASS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
