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

/* $Id:$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "php_v8js_macros.h"

static void v8js_fetch_php_variable(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) /* {{{ */
{
    v8::Handle<v8::External> data = v8::Handle<v8::External>::Cast(info.Data());
    v8js_accessor_ctx *ctx = static_cast<v8js_accessor_ctx *>(data->Value());
	v8::Isolate *isolate = ctx->isolate;
	zval *variable;

	V8JS_TSRMLS_FETCH();

	zend_is_auto_global(ctx->variable_name TSRMLS_CC);

	if ((variable = zend_hash_find(&EG(symbol_table), ctx->variable_name))) {
		info.GetReturnValue().Set(zval_to_v8js(variable, isolate TSRMLS_CC));
		return;
	}
}
/* }}} */

void v8js_accessor_ctx_dtor(v8js_accessor_ctx *ctx TSRMLS_DC) /* {{{ */
{
	zend_string_release(ctx->variable_name);
	efree(ctx);
}
/* }}} */

void v8js_register_accessors(std::vector<v8js_accessor_ctx*> *accessor_list, v8::Local<v8::FunctionTemplate> php_obj_t, zval *array, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	zend_string *property_name;
	zval *item;
	v8::Local<v8::ObjectTemplate> php_obj = php_obj_t->InstanceTemplate();

	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(array), property_name, item) {
		switch (Z_TYPE_P(item))
		{
			/*
			case IS_OBJECT:
			case IS_ARRAY:
			*/
			case IS_STRING:
				break;

			default:
				continue; /* Ignore invalid values */
		}

        // Create context to store accessor data
        v8js_accessor_ctx *ctx = (v8js_accessor_ctx *)emalloc(sizeof(v8js_accessor_ctx));
        ctx->variable_name = zend_string_copy(Z_STR_P(item));
        ctx->isolate = isolate;

		/* Set the variable fetch callback for given symbol on named property */
		php_obj->SetAccessor(V8JS_ZSTR(property_name), v8js_fetch_php_variable, NULL, v8::External::New(isolate, ctx), v8::PROHIBITS_OVERWRITING, v8::ReadOnly, v8::AccessorSignature::New(isolate, php_obj_t));

		/* record the context so we can free it later */
		accessor_list->push_back(ctx);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
