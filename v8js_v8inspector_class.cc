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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"
#include "v8js_exceptions.h"
#include "v8js_v8inspector_class.h"


extern "C" {
// #include "ext/date/php_date.h"
// #include "ext/standard/php_string.h"
// #include "zend_interfaces.h"
// #include "zend_closures.h"
// #include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}


/* {{{ Class Entries */
zend_class_entry *php_ce_v8inspector;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_v8inspector_handlers;
/* }}} */



static void v8js_v8inspector_free_storage(zend_object *object) /* {{{ */
{
	v8js_v8inspector *c = v8js_v8inspector_fetch_object(object);

    // TODO need to clean up?

	zend_object_std_dtor(&c->std);

	/* if(c->ctx) {
		c->v8obj.Reset();
		c->ctx->v8js_v8inspectors.remove(c);
	} */
}
/* }}} */

static zend_object *v8js_v8inspector_new(zend_class_entry *ce) /* {{{ */
{
	v8js_v8inspector *c;
	c = (v8js_v8inspector *) ecalloc(1, sizeof(v8js_v8inspector) + zend_object_properties_size(ce));

	zend_object_std_init(&c->std, ce);
	object_properties_init(&c->std, ce);

	c->std.handlers = &v8js_v8inspector_handlers;
	// new(&c->v8obj) v8::Persistent<v8::Value>();

	return &c->std;
}
/* }}} */


/* {{{ proto V8Inspector::__construct()
 */
PHP_METHOD(V8Inspector,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8Inspector objects!", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Inspector::__sleep()
 */
PHP_METHOD(V8Inspector, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Inspector instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Inspector::__wakeup()
 */
PHP_METHOD(V8Inspector, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Inspector instances", 0);
	RETURN_FALSE;
}
/* }}} */

void v8js_v8inspector_create(zval *res, v8js_ctx *ctx) /* {{{ */
{
    object_init_ex(res, php_ce_v8inspector);
	// v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(res);
}
/* }}} */

static const zend_function_entry v8js_v8inspector_methods[] = { /* {{{ */
	PHP_ME(V8Inspector,	__construct,			NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Inspector,	__sleep,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Inspector,	__wakeup,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(v8js_v8inspector_class) /* {{{ */
{
	zend_class_entry ce;

	/* V8Inspector Class */
	INIT_CLASS_ENTRY(ce, "V8Inspector", v8js_v8inspector_methods);
	php_ce_v8inspector = zend_register_internal_class(&ce);
	php_ce_v8inspector->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8inspector->create_object = v8js_v8inspector_new;

	/* V8Inspector handlers */
	memcpy(&v8js_v8inspector_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8js_v8inspector_handlers.clone_obj = NULL;
	v8js_v8inspector_handlers.cast_object = NULL;
	v8js_v8inspector_handlers.offset = XtOffsetOf(struct v8js_v8inspector, std);
	v8js_v8inspector_handlers.free_obj = v8js_v8inspector_free_storage;

	return SUCCESS;
} /* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
