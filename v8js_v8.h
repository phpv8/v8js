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

#ifndef V8JS_V8_H
#define V8JS_V8_H

void v8js_v8_init(TSRMLS_D);
void v8js_v8_call(php_v8js_ctx *c, zval **return_value,
				  long flags, long time_limit, long memory_limit,
				  std::function< v8::Local<v8::Value>(v8::Isolate *) >& v8_call TSRMLS_DC);
void v8js_terminate_execution(php_v8js_ctx *c TSRMLS_DC);

/* Fetch V8 object properties */
int v8js_get_properties_hash(v8::Handle<v8::Value> jsValue, HashTable *retval, int flags, v8::Isolate *isolate TSRMLS_DC);

#define V8JS_CTX_PROLOGUE(ctx) \
	if (!V8JSG(v8_initialized)) { \
		zend_error(E_ERROR, "V8 not initialized"); \
		return; \
	} \
	\
	v8::Isolate *isolate = (ctx)->isolate; \
	v8::Locker locker(isolate); \
	v8::Isolate::Scope isolate_scope(isolate); \
	v8::HandleScope handle_scope(isolate); \
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, (ctx)->context); \
	v8::Context::Scope context_scope(v8_context);

#define V8JS_BEGIN_CTX(ctx, object) \
	php_v8js_ctx *(ctx); \
	(ctx) = (php_v8js_ctx *) zend_object_store_get_object(object TSRMLS_CC); \
	V8JS_CTX_PROLOGUE(ctx);


#endif /* V8JS_V8_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
