/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2020 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifndef V8JS_V8INSPECTOR_CLASS_H
#define V8JS_V8INSPECTOR_CLASS_H

class InspectorClient;

/* {{{ Object container */
struct v8js_v8inspector {
	InspectorClient *client;
	zend_object std;
};
/* }}} */

extern zend_class_entry *php_ce_v8inspector;

/* Create PHP V8Inspector object */
void v8js_v8inspector_create(zval *res, v8js_ctx *ctx);

static inline v8js_v8inspector *v8js_v8inspector_fetch_object(zend_object *obj) {
	return (v8js_v8inspector *)((char *)obj - XtOffsetOf(struct v8js_v8inspector, std));
}

#define Z_V8JS_V8INSPECTOR_OBJ_P(zv) v8js_v8inspector_fetch_object(Z_OBJ_P(zv));



PHP_MINIT_FUNCTION(v8js_v8inspector_class);

#endif /* V8JS_V8INSPECTOR_CLASS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

