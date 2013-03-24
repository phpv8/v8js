/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
}

#include "php_v8js_macros.h"
#include <v8.h>
#include <map>
#include <vector>

/* global.exit - terminate execution */
V8JS_METHOD(exit) /* {{{ */
{
	v8::V8::TerminateExecution();
	return v8::Undefined();
}
/* }}} */

/* global.sleep - sleep for passed seconds */
V8JS_METHOD(sleep) /* {{{ */
{
	php_sleep(args[0]->Int32Value());
	return v8::Undefined();
}
/* }}} */

/* global.print - php print() */
V8JS_METHOD(print) /* {{{ */
{
	int ret = 0;
	TSRMLS_FETCH();

	for (int i = 0; i < args.Length(); i++) {
		v8::String::Utf8Value str(args[i]);
		const char *cstr = ToCString(str);
		ret = PHPWRITE(cstr, strlen(cstr));
	}
	return V8JS_INT(ret);
}
/* }}} */

static void _php_v8js_dumper(v8::Local<v8::Value> var, int level TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value str(var->ToDetailString());
	const char *valstr = ToCString(str);
	size_t valstr_len = (valstr) ? strlen(valstr) : 0;

	if (level > 1) {
		php_printf("%*c", (level - 1) * 2, ' ');
	}

	if (var->IsString())
	{
		php_printf("string(%d) \"%s\"\n", valstr_len, valstr);
	}
	else if (var->IsBoolean())
	{
		php_printf("bool(%s)\n", valstr);
	}
	else if (var->IsInt32() || var->IsUint32())
	{
		php_printf("int(%s)\n", valstr);
	}
	else if (var->IsNumber())
	{
		php_printf("float(%s)\n", valstr);
	}
	else if (var->IsDate())
	{
		php_printf("Date(%s)\n", valstr);
	}
#if PHP_V8_API_VERSION >= 2003007
	else if (var->IsRegExp())
	{
		php_printf("RegExp(%s)\n", valstr);
	}
#endif
	else if (var->IsArray())
	{
		v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(var);
		uint32_t length = array->Length();

		php_printf("array(%d) {\n", length);

		for (unsigned i = 0; i < length; i++) {
			php_printf("%*c[%d] =>\n", level * 2, ' ', i);
			_php_v8js_dumper(array->Get(i), level + 1 TSRMLS_CC);
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

		if (var->IsFunction())
		{
			v8::String::Utf8Value csource(object->ToString());
			php_printf("object(%s)#%d {\n%*c%s\n", ToCString(cname), object->GetIdentityHash(), level * 2 + 2, ' ', ToCString(csource));
		}
		else
		{
			v8::Local<v8::Array> keys = object->GetPropertyNames();
			uint32_t length = keys->Length();

			php_printf("object(%s)#%d (%d) {\n", ToCString(cname), object->GetIdentityHash(), length);

			for (unsigned i = 0; i < length; i++) {
				v8::Local<v8::String> key = keys->Get(i)->ToString();
				v8::String::Utf8Value kname(key);
				php_printf("%*c[\"%s\"] =>\n", level * 2, ' ', ToCString(kname));
				_php_v8js_dumper(object->Get(key), level + 1 TSRMLS_CC);
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
	int i;
	TSRMLS_FETCH();

	for (int i = 0; i < args.Length(); i++) {
		_php_v8js_dumper(args[i], 1 TSRMLS_CC);
	}
	
	return V8JS_NULL;
}
/* }}} */

// TODO: Put this in php_v8js_context
std::map<char *, v8::Handle<v8::Object> > modules_loaded;
std::vector<char *> modules_stack;

void split_terms(char *identifier, std::vector<char *> &terms)
{
	char *term = (char *)malloc(PATH_MAX), *ptr = term;

	// Initialise the term string
	*term = 0;

	while (*identifier > 0) {
		if (*identifier == '/') {
			if (strlen(term) > 0) {
				// Terminate term string and add to terms vector
				*ptr++ = 0;
				terms.push_back(strdup(term));

				// Reset term string
				memset(term, 0, strlen(term));
				ptr = term;
			}
		} else {
			*ptr++ = *identifier;
		}

		identifier++;
	}

	if (strlen(term) > 0) {
		// Terminate term string and add to terms vector
		*ptr++ = 0;
		terms.push_back(strdup(term));
	}

	if (term > 0) {
		free(term);
	}
}

void normalize_identifier(char *identifier, char *normalised)
{
	std::vector<char *> terms;
	split_terms(identifier, terms);

	std::vector<char *> normalised_terms;

	for (std::vector<char *>::iterator it = terms.begin(); it != terms.end(); it++) {
		char *term = *it;

		if (!strcmp(term, "..")) {
			normalised_terms.pop_back();
		} else if (strcmp(term, ".")) {
			normalised_terms.push_back(term);
		}
	}

	// Initialise the normalised string
	*normalised = 0;

	for (std::vector<char *>::iterator it = normalised_terms.begin(); it != normalised_terms.end(); it++) {
		char *term = *it;

		if (strlen(normalised) > 0) {
			strcat(normalised, "/");
		}

		strcat(normalised, term);
	}
}

V8JS_METHOD(require)
{
	v8::String::Utf8Value module_name_v8(args[0]);

	// Make sure to duplicate the module name string so it doesn't get freed by the V8 garbage collector
	char *module_name = strdup(ToCString(module_name_v8));
	char *normalised_module_identifier = (char *)malloc(strlen(module_name));

	normalize_identifier(module_name, normalised_module_identifier);

	// Check for module cyclic dependencies
	for (std::vector<char *>::iterator it = modules_stack.begin(); it != modules_stack.end(); ++it)
    {
    	if (!strcmp(*it, normalised_module_identifier)) {
    		return v8::ThrowException(v8::String::New("Module cyclic dependency"));
    	}
    }

    // If we have already loaded and cached this module then use it
	if (modules_loaded.count(normalised_module_identifier) > 0) {
		return modules_loaded[normalised_module_identifier];
	}

	// Get the extension context
	v8::Handle<v8::External> data = v8::Handle<v8::External>::Cast(args.Data());
	php_v8js_ctx *c = static_cast<php_v8js_ctx*>(data->Value());

	// Check that we have a module loader
	if (c->module_loader == NULL) {
		return v8::ThrowException(v8::String::New("No module loader"));
	}

	// Callback to PHP to load the module code

	zval module_code;
	zval *module_name_zend;

	MAKE_STD_ZVAL(module_name_zend);
	ZVAL_STRING(module_name_zend, normalised_module_identifier, 1);
	zval* params[] = { module_name_zend };

	if (FAILURE == call_user_function(EG(function_table), NULL, c->module_loader, &module_code, 1, params TSRMLS_CC)) {
		return v8::ThrowException(v8::String::New("Module loader callback failed"));
	}

	// Convert the return value to string
	if (Z_TYPE(module_code) != IS_STRING) {
    	convert_to_string(&module_code);
	}

	// Check that some code has been returned
	if (!strlen(Z_STRVAL(module_code))) {
		return v8::ThrowException(v8::String::New("Module loader callback did not return code"));
	}

	// Create a template for the global object and set the built-in global functions
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(V8JS_MN(print)), v8::ReadOnly);
	global->Set(v8::String::New("require"), v8::FunctionTemplate::New(V8JS_MN(require), v8::External::New(c)), v8::ReadOnly);

	// Add the exports object in which the module returns its API
	v8::Handle<v8::ObjectTemplate> exports_template = v8::ObjectTemplate::New();
	v8::Handle<v8::Object> exports = exports_template->NewInstance();
	global->Set(v8::String::New("exports"), exports);

	// Each module gets its own context so different modules do not affect each other
	v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);

	// Catch JS exceptions
	v8::TryCatch try_catch;

	// Enter the module context
	v8::Context::Scope scope(context);

	v8::HandleScope handle_scope;

	// Set script identifier
	v8::Local<v8::String> sname = V8JS_SYM("require");

	v8::Local<v8::String> source = v8::String::New(Z_STRVAL(module_code));

	// Create and compile script
	v8::Local<v8::Script> script = v8::Script::New(source, sname);

	// The script will be empty if there are compile errors
	if (script.IsEmpty()) {
		return v8::ThrowException(v8::String::New("Module script compile failed"));
	}

	// Add this module to the stack
	modules_stack.push_back(normalised_module_identifier);

	// Run script
	v8::Local<v8::Value> result = script->Run();

	// Remove this module from the stack
	modules_stack.pop_back();

	// Script possibly terminated, return immediately
	if (!try_catch.CanContinue()) {
		return v8::ThrowException(v8::String::New("Module script compile failed"));
	}

	// Handle runtime JS exceptions
	if (try_catch.HasCaught()) {

		// Rethrow the exception back to JS
		return try_catch.ReThrow();
	}

	// Cache the module so it doesn't need to be compiled and run again
	modules_loaded[normalised_module_identifier] = handle_scope.Close(exports);

	return modules_loaded[normalised_module_identifier];
}

void php_v8js_register_methods(v8::Handle<v8::ObjectTemplate> global, php_v8js_ctx *c) /* {{{ */
{
	global->Set(V8JS_SYM("exit"), v8::FunctionTemplate::New(V8JS_MN(exit)), v8::ReadOnly);
	global->Set(V8JS_SYM("sleep"), v8::FunctionTemplate::New(V8JS_MN(sleep)), v8::ReadOnly);
	global->Set(V8JS_SYM("print"), v8::FunctionTemplate::New(V8JS_MN(print)), v8::ReadOnly);
	global->Set(V8JS_SYM("var_dump"), v8::FunctionTemplate::New(V8JS_MN(var_dump)), v8::ReadOnly);

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
