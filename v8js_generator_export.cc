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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include "php_v8js_macros.h"

v8::Local<v8::Value> v8js_wrap_generator(v8::Isolate *isolate, v8::Local<v8::Value> wrapped_object) /* {{{ */
{
	v8::Local<v8::Value> result;

	assert(!wrapped_object.IsEmpty());
	assert(wrapped_object->IsObject());

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
    v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, ctx->context);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::String> source = V8JS_SYM("(\
function(wrapped_object) {					\
	return (function*() {					\
		for(;;) {							\
			if(!wrapped_object.valid()) {	\
				return;						\
			}								\
			yield wrapped_object.current();	\
			wrapped_object.next();			\
		}									\
	})();									\
})");
	v8::MaybeLocal<v8::Script> script = v8::Script::Compile(v8_context, source);

	if(script.IsEmpty()) {
		zend_error(E_ERROR, "Failed to compile Generator object wrapper");
		return result;
	}

	v8::MaybeLocal<v8::Value> wrapper_fn_val = script.ToLocalChecked()->Run(v8_context);

	if(wrapper_fn_val.IsEmpty() || !wrapper_fn_val.ToLocalChecked()->IsFunction()) {
		zend_error(E_ERROR, "Failed to create Generator object wrapper function");
		return result;
	}

	v8::Local<v8::Function> wrapper_fn = v8::Local<v8::Function>::Cast(wrapper_fn_val.ToLocalChecked());
	v8::Local<v8::Value> *jsArgv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>)));

	new(&jsArgv[0]) v8::Local<v8::Value>;
	jsArgv[0] = v8::Local<v8::Value>::New(isolate, wrapped_object);

	wrapper_fn->Call(v8_context, V8JS_GLOBAL(isolate), 1, jsArgv).ToLocal(&result);
	return result;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
