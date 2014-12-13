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

/* Helper macros */
#define V8JS_SYM(v)			v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, sizeof(v) - 1)
#define V8JS_SYML(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, l)
#define V8JS_STR(v)			v8::String::NewFromUtf8(isolate, v)
#define V8JS_STRL(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kNormalString, l)
#define V8JS_INT(v)			v8::Integer::New(isolate, v)
#define V8JS_UINT(v)		v8::Integer::NewFromUnsigned(isolate, v)
#define V8JS_FLOAT(v)		v8::Number::New(isolate, v)
#define V8JS_BOOL(v)		((v)?v8::True(isolate):v8::False(isolate))
#define V8JS_DATE(v)		v8::Date::New(isolate, v)
#define V8JS_NULL			v8::Null(isolate)
#define V8JS_UNDEFINED		v8::Undefined(isolate)
#define V8JS_MN(name)		v8js_method_##name
#define V8JS_METHOD(name)	void V8JS_MN(name)(const v8::FunctionCallbackInfo<v8::Value>& info)
#define V8JS_THROW(isolate, type, message, message_len)	(isolate)->ThrowException(v8::Exception::type(V8JS_STRL(message, message_len)))
#define V8JS_GLOBAL(isolate)			((isolate)->GetCurrentContext()->Global())


/* Extracts a C string from a V8 Utf8Value. */
static inline const char * ToCString(const v8::String::Utf8Value &value) /* {{{ */
{
	return *value ? *value : "<string conversion failed>";
}
/* }}} */



void v8js_v8_init(TSRMLS_D);
void v8js_v8_call(v8js_ctx *c, zval **return_value,
				  long flags, long time_limit, long memory_limit,
				  std::function< v8::Local<v8::Value>(v8::Isolate *) >& v8_call TSRMLS_DC);
void v8js_terminate_execution(v8js_ctx *c TSRMLS_DC);

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
	v8js_ctx *(ctx); \
	(ctx) = (v8js_ctx *) zend_object_store_get_object(object TSRMLS_CC); \
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
