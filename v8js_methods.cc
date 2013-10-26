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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"

extern "C" {
#include "zend_exceptions.h"
}

/* Forward declarations */
void php_v8js_commonjs_normalise_identifier(char *base, char *identifier, char *normalised_path, char *module_name);

/* global.exit - terminate execution */
V8JS_METHOD(exit) /* {{{ */
{
	v8::V8::TerminateExecution();
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
	int ret = 0;
	V8JS_TSRMLS_FETCH();

	for (int i = 0; i < info.Length(); i++) {
		v8::String::Utf8Value str(info[i]);
		const char *cstr = ToCString(str);
		ret = PHPWRITE(cstr, strlen(cstr));
	}
	info.GetReturnValue().Set(V8JS_INT(ret));
}
/* }}} */

static void _php_v8js_dumper(v8::Isolate *isolate, v8::Local<v8::Value> var, int level TSRMLS_DC) /* {{{ */
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

	v8::TryCatch try_catch; /* object.toString() can throw an exception */
	v8::Local<v8::String> details = var->ToDetailString();
	if (try_catch.HasCaught()) {
		details = V8JS_SYM("<toString threw exception>");
	}
	v8::String::Utf8Value str(details);
	const char *valstr = ToCString(str);
	size_t valstr_len = (valstr) ? strlen(valstr) : 0;

	if (var->IsString())
	{
		php_printf("string(%zu) \"%s\"\n", valstr_len, valstr);
	}
	else if (var->IsDate())
	{
		// fake the fields of a PHP DateTime
		php_printf("Date(%s)\n", valstr);
	}
#if PHP_V8_API_VERSION >= 2003007
	else if (var->IsRegExp())
	{
		php_printf("regexp(%s)\n", valstr);
	}
#endif
	else if (var->IsArray())
	{
		v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(var);
		uint32_t length = array->Length();

		php_printf("array(%d) {\n", length);

		for (unsigned i = 0; i < length; i++) {
			php_printf("%*c[%d] =>\n", level * 2, ' ', i);
			_php_v8js_dumper(isolate, array->Get(i), level + 1 TSRMLS_CC);
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

		if (var->IsFunction())
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
				_php_v8js_dumper(isolate, object->Get(key), level + 1 TSRMLS_CC);
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
	V8JS_TSRMLS_FETCH();

	for (int i = 0; i < info.Length(); i++) {
		_php_v8js_dumper(isolate, info[i], 1 TSRMLS_CC);
	}

	info.GetReturnValue().Set(V8JS_NULL);
}
/* }}} */

V8JS_METHOD(require)
{
	v8::Isolate *isolate = info.GetIsolate();
	V8JS_TSRMLS_FETCH();

	// Get the extension context
	v8::Handle<v8::External> data = v8::Handle<v8::External>::Cast(info.Data());
	php_v8js_ctx *c = static_cast<php_v8js_ctx*>(data->Value());

	// Check that we have a module loader
	if (c->module_loader == NULL) {
		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("No module loader")));
		return;
	}

	v8::String::Utf8Value module_id_v8(info[0]);

	// Make sure to duplicate the module name string so it doesn't get freed by the V8 garbage collector
	char *module_id = estrdup(ToCString(module_id_v8));
	char *normalised_path = (char *)emalloc(PATH_MAX);
	char *module_name = (char *)emalloc(PATH_MAX);

	php_v8js_commonjs_normalise_identifier(c->modules_base.back(), module_id, normalised_path, module_name);
	efree(module_id);

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
    		efree(normalised_path);

		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module cyclic dependency")));
		return;
    	}
    }

    // If we have already loaded and cached this module then use it
	if (V8JSG(modules_loaded).count(normalised_module_id) > 0) {
		efree(normalised_path);

		info.GetReturnValue().Set(V8JSG(modules_loaded)[normalised_module_id]);
		return;
	}

	// Callback to PHP to load the module code

	zval module_code;
	zval *normalised_path_zend;

	MAKE_STD_ZVAL(normalised_path_zend);
	ZVAL_STRING(normalised_path_zend, normalised_module_id, 1);
	zval* params[] = { normalised_path_zend };

	if (FAILURE == call_user_function(EG(function_table), NULL, c->module_loader, &module_code, 1, params TSRMLS_CC)) {
		efree(normalised_path);

		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module loader callback failed")));
		return;
	}

	// Check if an exception was thrown
	if (EG(exception)) {
		efree(normalised_path);

		// Clear the PHP exception and throw it in V8 instead
		zend_clear_exception(TSRMLS_C);
		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module loader callback exception")));
		return;
	}

	// Convert the return value to string
	if (Z_TYPE(module_code) != IS_STRING) {
    	convert_to_string(&module_code);
	}

	// Check that some code has been returned
	if (!strlen(Z_STRVAL(module_code))) {
		efree(normalised_path);

		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module loader callback did not return code")));
		return;
	}

	// Create a template for the global object and set the built-in global functions
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	global->Set(V8JS_SYM("print"), v8::FunctionTemplate::New(V8JS_MN(print)), v8::ReadOnly);
	global->Set(V8JS_SYM("sleep"), v8::FunctionTemplate::New(V8JS_MN(sleep)), v8::ReadOnly);
	global->Set(V8JS_SYM("require"), v8::FunctionTemplate::New(V8JS_MN(require), v8::External::New(c)), v8::ReadOnly);

	// Add the exports object in which the module can return its API
	v8::Handle<v8::ObjectTemplate> exports_template = v8::ObjectTemplate::New();
	v8::Handle<v8::Object> exports = exports_template->NewInstance();
	global->Set(V8JS_SYM("exports"), exports);

	// Add the module object in which the module can have more fine-grained control over what it can return
	v8::Handle<v8::ObjectTemplate> module_template = v8::ObjectTemplate::New();
	v8::Handle<v8::Object> module = module_template->NewInstance();
	module->Set(V8JS_SYM("id"), V8JS_STR(normalised_module_id));
	global->Set(V8JS_SYM("module"), module);

	// Each module gets its own context so different modules do not affect each other
	v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, v8::Context::New(isolate, NULL, global));

	// Catch JS exceptions
	v8::TryCatch try_catch;

	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);

	v8::HandleScope handle_scope(isolate);

	// Enter the module context
	v8::Context::Scope scope(context);
	// Set script identifier
	v8::Local<v8::String> sname = V8JS_SYM("require");

	v8::Local<v8::String> source = V8JS_STR(Z_STRVAL(module_code));

	// Create and compile script
	v8::Local<v8::Script> script = v8::Script::New(source, sname);

	// The script will be empty if there are compile errors
	if (script.IsEmpty()) {
		efree(normalised_path);
		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module script compile failed")));
		return;
	}

	// Add this module and path to the stack
	c->modules_stack.push_back(normalised_module_id);

	c->modules_base.push_back(normalised_path);

	// Run script
	v8::Local<v8::Value> result = script->Run();

	// Remove this module and path from the stack
	c->modules_stack.pop_back();
	c->modules_base.pop_back();

	efree(normalised_path);

	// Script possibly terminated, return immediately
	if (!try_catch.CanContinue()) {
		info.GetReturnValue().Set(v8::ThrowException(V8JS_SYM("Module script compile failed")));
		return;
	}

	// Handle runtime JS exceptions
	if (try_catch.HasCaught()) {
		// Rethrow the exception back to JS
		info.GetReturnValue().Set(try_catch.ReThrow());
		return;
	}

	// Cache the module so it doesn't need to be compiled and run again
	// Ensure compatibility with CommonJS implementations such as NodeJS by playing nicely with module.exports and exports
	if (module->Has(V8JS_SYM("exports")) && !module->Get(V8JS_SYM("exports"))->IsUndefined()) {
		// If module.exports has been set then we cache this arbitrary value...
		V8JSG(modules_loaded)[normalised_module_id] = handle_scope.Close(module->Get(V8JS_SYM("exports"))->ToObject());
	} else {
		// ...otherwise we cache the exports object itself
		V8JSG(modules_loaded)[normalised_module_id] = handle_scope.Close(exports);
	}

	info.GetReturnValue().Set(V8JSG(modules_loaded)[normalised_module_id]);
}

void php_v8js_register_methods(v8::Handle<v8::ObjectTemplate> global, php_v8js_ctx *c) /* {{{ */
{
	v8::Isolate *isolate = c->isolate;
	global->Set(V8JS_SYM("exit"), v8::FunctionTemplate::New(V8JS_MN(exit)), v8::ReadOnly);
	global->Set(V8JS_SYM("sleep"), v8::FunctionTemplate::New(V8JS_MN(sleep)), v8::ReadOnly);
	global->Set(V8JS_SYM("print"), v8::FunctionTemplate::New(V8JS_MN(print)), v8::ReadOnly);
	global->Set(V8JS_SYM("var_dump"), v8::FunctionTemplate::New(V8JS_MN(var_dump)), v8::ReadOnly);

	c->modules_base.push_back("");
	global->Set(V8JS_SYM("require"), v8::FunctionTemplate::New(V8JS_MN(require), v8::External::New(c)), v8::ReadOnly);
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
