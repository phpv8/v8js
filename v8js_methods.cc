/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"
#include "v8js_commonjs.h"
#include "v8js_exceptions.h"

extern "C" {
#include "zend_exceptions.h"
}

/* global.exit - terminate execution */
V8JS_METHOD(exit) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8js_terminate_execution(isolate);
}
/* }}} */

/* global.sleep - sleep for passed seconds */
V8JS_METHOD(sleep) /* {{{ */
{
	php_sleep(info[0]->Int32Value());
}
/* }}} */

/* global.print - php print() */
V8JS_METHOD(print) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	zend_long ret = 0;

	for (int i = 0; i < info.Length(); i++) {
		v8::String::Utf8Value str(info[i]);
		const char *cstr = ToCString(str);
		ret = PHPWRITE(cstr, strlen(cstr));
	}

	info.GetReturnValue().Set(zend_long_to_v8js(ret, isolate));
}
/* }}} */

static void v8js_dumper(v8::Isolate *isolate, v8::Local<v8::Value> var, int level) /* {{{ */
{
	if (level > 1) {
		php_printf("%*c", (level - 1) * 2, ' ');
	}

	if (var.IsEmpty())
	{
		php_printf("<empty>\n");
		return;
	}
	if (var->IsNull() || var->IsUndefined() /* PHP compat */)
	{
		php_printf("NULL\n");
		return;
	}
	if (var->IsInt32())
	{
		php_printf("int(%ld)\n", (long) var->IntegerValue());
		return;
	}
	if (var->IsUint32())
	{
		php_printf("int(%lu)\n", (unsigned long) var->IntegerValue());
		return;
	}
	if (var->IsNumber())
	{
		php_printf("float(%f)\n", var->NumberValue());
		return;
	}
	if (var->IsBoolean())
	{
		php_printf("bool(%s)\n", var->BooleanValue() ? "true" : "false");
		return;
	}

	v8::TryCatch try_catch(isolate); /* object.toString() can throw an exception */
	v8::Local<v8::String> details;

	if(var->IsRegExp()) {
		v8::RegExp *re = v8::RegExp::Cast(*var);
		details = re->GetSource();
	}
	else {
		details = var->ToDetailString(isolate->GetEnteredContext()).FromMaybe(v8::Local<v8::String>());

		if (try_catch.HasCaught()) {
			details = V8JS_SYM("<toString threw exception>");
		}
	}

	v8::String::Utf8Value str(details);
	const char *valstr = ToCString(str);
	size_t valstr_len = details->ToString()->Utf8Length();

	if (var->IsString())
	{
		php_printf("string(%zu) \"", valstr_len);
		PHPWRITE(valstr, valstr_len);
		php_printf("\"\n");
	}
	else if (var->IsDate())
	{
		// fake the fields of a PHP DateTime
		php_printf("Date(%s)\n", valstr);
	}
	else if (var->IsRegExp())
	{
		php_printf("regexp(/%s/)\n", valstr);
	}
	else if (var->IsArray())
	{
		v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(var);
		uint32_t length = array->Length();

		php_printf("array(%d) {\n", length);

		for (unsigned i = 0; i < length; i++) {
			php_printf("%*c[%d] =>\n", level * 2, ' ', i);
			v8js_dumper(isolate, array->Get(i), level + 1);
		}

		if (level > 1) {
			php_printf("%*c", (level - 1) * 2, ' ');
		}

		ZEND_PUTS("}\n");
	}
	else if (var->IsObject())
	{
		v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(var);
		V8JS_GET_CLASS_NAME(cname, object);
		int hash = object->GetIdentityHash();

		if (var->IsFunction() && strcmp(ToCString(cname), "Closure") != 0)
		{
			v8::String::Utf8Value csource(object->ToString());
			php_printf("object(Closure)#%d {\n%*c%s\n", hash, level * 2 + 2, ' ', ToCString(csource));
		}
		else
		{
			v8::Local<v8::Array> keys = object->GetOwnPropertyNames();
			uint32_t length = keys->Length();

			if (strcmp(ToCString(cname), "Array") == 0 ||
				strcmp(ToCString(cname), "V8Object") == 0) {
				php_printf("array");
			} else {
				php_printf("object(%s)#%d", ToCString(cname), hash);
			}
			php_printf(" (%d) {\n", length);

			for (unsigned i = 0; i < length; i++) {
				v8::Local<v8::String> key = keys->Get(i)->ToString();
				v8::String::Utf8Value kname(key);
				php_printf("%*c[\"%s\"] =>\n", level * 2, ' ', ToCString(kname));
				v8js_dumper(isolate, object->Get(key), level + 1);
			}
		}

		if (level > 1) {
			php_printf("%*c", (level - 1) * 2, ' ');
		}

		ZEND_PUTS("}\n");
	}
	else /* null, undefined, etc. */
	{
		php_printf("<%s>\n", valstr);
	}
}
/* }}} */

/* global.var_dump - Dump JS values */
V8JS_METHOD(var_dump) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();

	for (int i = 0; i < info.Length(); i++) {
		v8js_dumper(isolate, info[i], 1);
	}

	info.GetReturnValue().Set(V8JS_NULL);
}
/* }}} */

V8JS_METHOD(require)
{
	v8::Isolate *isolate = info.GetIsolate();

	// Get the extension context
	v8::Local<v8::External> data = v8::Local<v8::External>::Cast(info.Data());
	v8js_ctx *c = static_cast<v8js_ctx*>(data->Value());

	// Check that we have a module loader
	if(Z_TYPE(c->module_loader) == IS_NULL) {
		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("No module loader")));
		return;
	}

	v8::String::Utf8Value module_id_v8(info[0]);
	const char *module_id = ToCString(module_id_v8);
	char *normalised_path, *module_name;

	if (Z_TYPE(c->module_normaliser) == IS_NULL) {
		// No custom normalisation routine registered, use internal one
		normalised_path = (char *)emalloc(PATH_MAX);
		module_name = (char *)emalloc(PATH_MAX);

		v8js_commonjs_normalise_identifier(c->modules_base.back(), module_id, normalised_path, module_name);
	}
	else {
		// Call custom normaliser
		int call_result;
		zval params[2];
		zval normaliser_result;

		zend_try {
			{
				isolate->Exit();
				v8::Unlocker unlocker(isolate);

				ZVAL_STRING(&params[0], c->modules_base.back());
				ZVAL_STRING(&params[1], module_id);

				call_result = call_user_function_ex(EG(function_table), NULL, &c->module_normaliser,
													&normaliser_result, 2, params, 0, NULL);
			}

			isolate->Enter();

			if (call_result == FAILURE) {
				info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module normaliser callback failed")));
			}
		}
		zend_catch {
			v8js_terminate_execution(isolate);
			V8JSG(fatal_error_abort) = 1;
			call_result = FAILURE;
		}
		zend_end_try();

		zval_ptr_dtor(&params[0]);
		zval_ptr_dtor(&params[1]);

		if(call_result == FAILURE) {
			return;
		}

		// Check if an exception was thrown
		if (EG(exception)) {
			if (c->flags & V8JS_FLAG_PROPAGATE_PHP_EXCEPTIONS) {
				zval tmp_zv;
				ZVAL_OBJ(&tmp_zv, EG(exception));
				info.GetReturnValue().Set(isolate->ThrowException(zval_to_v8js(&tmp_zv, isolate)));
				zend_clear_exception();
			} else {
				v8js_terminate_execution(isolate);
			}
			return;
		}

		if (Z_TYPE(normaliser_result) != IS_ARRAY) {
			zval_ptr_dtor(&normaliser_result);
			info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module normaliser didn't return an array")));
			return;
		}

		HashTable *ht = HASH_OF(&normaliser_result);
		int num_elements = zend_hash_num_elements(ht);

		if(num_elements != 2) {
			zval_ptr_dtor(&normaliser_result);
			info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module normaliser expected to return array of 2 strings")));
			return;
		}

		zval *data;
		ulong index = 0;

		ZEND_HASH_FOREACH_VAL(ht, data) {
			if (Z_TYPE_P(data) != IS_STRING) {
				convert_to_string(data);
			}

			switch(index++) {
			case 0: // normalised path
				normalised_path = estrndup(Z_STRVAL_P(data), Z_STRLEN_P(data));
				break;

			case 1: // normalised module id
				module_name = estrndup(Z_STRVAL_P(data), Z_STRLEN_P(data));
				break;
			}
		}
		ZEND_HASH_FOREACH_END();

		zval_ptr_dtor(&normaliser_result);
	}

	char *normalised_module_id = (char *)emalloc(strlen(normalised_path)+1+strlen(module_name)+1);
	*normalised_module_id = 0;

	if (strlen(normalised_path) > 0) {
		strcat(normalised_module_id, normalised_path);
		strcat(normalised_module_id, "/");
	}

	strcat(normalised_module_id, module_name);
	efree(module_name);

	// Check for module cyclic dependencies
	for (std::vector<char *>::iterator it = c->modules_stack.begin(); it != c->modules_stack.end(); ++it)
    {
    	if (!strcmp(*it, normalised_module_id)) {
    		efree(normalised_module_id);
    		efree(normalised_path);

		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module cyclic dependency")));
		return;
    	}
    }

    // If we have already loaded and cached this module then use it
	if (c->modules_loaded.count(normalised_module_id) > 0) {
		v8::Persistent<v8::Value> newobj;
		newobj.Reset(isolate, c->modules_loaded[normalised_module_id]);
		info.GetReturnValue().Set(newobj);

		efree(normalised_module_id);
		efree(normalised_path);

		return;
	}

	// Callback to PHP to load the module code

	zval module_code;
	int call_result;
	zval params[1];

	{
		isolate->Exit();
		v8::Unlocker unlocker(isolate);

		zend_try {
			ZVAL_STRING(&params[0], normalised_module_id);
			call_result = call_user_function_ex(EG(function_table), NULL, &c->module_loader, &module_code, 1, params, 0, NULL);
		}
		zend_catch {
			v8js_terminate_execution(isolate);
			V8JSG(fatal_error_abort) = 1;
		}
		zend_end_try();
	}

	isolate->Enter();

	if (V8JSG(fatal_error_abort)) {
		call_result = FAILURE;
	}
	else if (call_result == FAILURE) {
		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module loader callback failed")));
	}

	zval_ptr_dtor(&params[0]);

	if (call_result == FAILURE) {
		efree(normalised_module_id);
		efree(normalised_path);
		return;
	}

	// Check if an exception was thrown
	if (EG(exception)) {
		efree(normalised_module_id);
		efree(normalised_path);

		if (c->flags & V8JS_FLAG_PROPAGATE_PHP_EXCEPTIONS) {
			zval tmp_zv;
			ZVAL_OBJ(&tmp_zv, EG(exception));
			info.GetReturnValue().Set(isolate->ThrowException(zval_to_v8js(&tmp_zv, isolate)));
			zend_clear_exception();
		} else {
			v8js_terminate_execution(isolate);
		}

		return;
	}

	// Convert the return value to string
	if (Z_TYPE(module_code) != IS_STRING) {
		convert_to_string(&module_code);
	}

	v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, c->context);

	// Catch JS exceptions
	v8::TryCatch try_catch(isolate);

	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);

	v8::HandleScope handle_scope(isolate);

	// Enter the module context
	v8::Context::Scope scope(context);

	// Set script identifier
	v8::Local<v8::String> sname = V8JS_STR(normalised_module_id);

	if (Z_STRLEN(module_code) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Module code size exceeds maximum supported length", 0);
		return;
	}

	v8::Local<v8::String> source = V8JS_ZSTR(Z_STR(module_code));
	zval_ptr_dtor(&module_code);

	source = v8::String::Concat(V8JS_SYM("(function (exports, module) {"), source);
	source = v8::String::Concat(source, V8JS_SYM("\n});"));

	// Create and compile script
	v8::Local<v8::Script> script = v8::Script::Compile(source, sname);

	// The script will be empty if there are compile errors
	if (script.IsEmpty()) {
		efree(normalised_module_id);
		efree(normalised_path);
		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module script compile failed")));
		return;
	}

	// Add this module and path to the stack
	c->modules_stack.push_back(normalised_module_id);
	c->modules_base.push_back(normalised_path);

	// Run script to evaluate closure
	v8::Local<v8::Value> module_function = script->Run();

	// Prepare exports & module object
	v8::Local<v8::Object> exports = v8::Object::New(isolate);

	v8::Local<v8::Object> module = v8::Object::New(isolate);
	module->Set(V8JS_SYM("id"), V8JS_STR(normalised_module_id));
	module->Set(V8JS_SYM("exports"), exports);

	if (module_function->IsFunction()) {
		v8::Local<v8::Value> *jsArgv = static_cast<v8::Local<v8::Value> *>(alloca(2 * sizeof(v8::Local<v8::Value>)));
		new(&jsArgv[0]) v8::Local<v8::Value>;
		jsArgv[0] = exports;

		new(&jsArgv[1]) v8::Local<v8::Value>;
		jsArgv[1] = module;

		// actually call the module
		v8::Local<v8::Function>::Cast(module_function)->Call(exports, 2, jsArgv);
	}

	// Remove this module and path from the stack
	c->modules_stack.pop_back();
	c->modules_base.pop_back();

	efree(normalised_path);

	if (!module_function->IsFunction()) {
		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Wrapped module script failed to return function")));
		efree(normalised_module_id);
		return;
	}

	// Script possibly terminated, return immediately
	if (!try_catch.CanContinue()) {
		info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Module script compile failed")));
		efree(normalised_module_id);
		return;
	}

	// Handle runtime JS exceptions
	if (try_catch.HasCaught()) {
		// Rethrow the exception back to JS
		info.GetReturnValue().Set(try_catch.ReThrow());
		efree(normalised_module_id);
		return;
	}

	v8::Local<v8::Value> newobj;

	// Cache the module so it doesn't need to be compiled and run again
	// Ensure compatibility with CommonJS implementations such as NodeJS by playing nicely with module.exports and exports
	if (module->Has(V8JS_SYM("exports"))) {
		newobj = module->Get(V8JS_SYM("exports"));
	}

	c->modules_loaded[normalised_module_id].Reset(isolate, newobj);
	info.GetReturnValue().Set(newobj);
}

void v8js_register_methods(v8::Local<v8::ObjectTemplate> global, v8js_ctx *c) /* {{{ */
{
	v8::Isolate *isolate = c->isolate;
	global->Set(V8JS_SYM("exit"), v8::FunctionTemplate::New(isolate, V8JS_MN(exit)), v8::ReadOnly);
	global->Set(V8JS_SYM("sleep"), v8::FunctionTemplate::New(isolate, V8JS_MN(sleep)), v8::ReadOnly);
	global->Set(V8JS_SYM("print"), v8::FunctionTemplate::New(isolate, V8JS_MN(print)), v8::ReadOnly);
	global->Set(V8JS_SYM("var_dump"), v8::FunctionTemplate::New(isolate, V8JS_MN(var_dump)), v8::ReadOnly);

	c->modules_base.push_back("");
	global->Set(V8JS_SYM("require"), v8::FunctionTemplate::New(isolate, V8JS_MN(require), v8::External::New(isolate, c)), v8::ReadOnly);
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
