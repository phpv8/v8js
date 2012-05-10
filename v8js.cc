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

/* $Id:$ */

#define V8JS_DEBUG 0

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_smart_str.h"
#include "zend_exceptions.h"
#include "php_v8js.h"
}

#include <v8.h>
#include "php_v8js_macros.h"

/* Forward declarations */
static void php_v8js_throw_exception(v8::TryCatch * TSRMLS_DC);
static void php_v8js_create_exception(zval *, v8::TryCatch * TSRMLS_DC);

/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(v8js)
	int v8_initialized;
	HashTable *extensions;
	v8::Persistent<v8::FunctionTemplate> global_template;
	int disposed_contexts; /* Disposed contexts since last time V8 did GC */

	/* Ini globals */
	char *v8_flags; /* V8 command line flags */
	int max_disposed_contexts; /* Max disposed context allowed before forcing V8 GC */
ZEND_END_MODULE_GLOBALS(v8js)

#ifdef ZTS
# define V8JSG(v) TSRMG(v8js_globals_id, zend_v8js_globals *, v)
#else
# define V8JSG(v) (v8js_globals.v)
#endif

ZEND_DECLARE_MODULE_GLOBALS(v8js)

/* {{{ INI Settings */

static ZEND_INI_MH(v8js_OnUpdateMaxDisposedContexts) /* {{{ */
{
	int max_disposed = zend_atoi(new_value, new_value_length);

	if (max_disposed <= 0) {
		return FAILURE;
	}

	V8JSG(max_disposed_contexts) = max_disposed;

	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateV8Flags) /* {{{ */
{
	if (new_value) {
		if (V8JSG(v8_flags)) {
			free(V8JSG(v8_flags));
			V8JSG(v8_flags) = NULL;
		}
		if (!new_value[0]) {
			return FAILURE;
		}
		V8JSG(v8_flags) = zend_strndup(new_value, new_value_length);
	}

	return SUCCESS;
}
/* }}} */

ZEND_INI_BEGIN() /* {{{ */
	ZEND_INI_ENTRY("v8js.max_disposed_contexts", "25", ZEND_INI_ALL, v8js_OnUpdateMaxDisposedContexts)
	ZEND_INI_ENTRY("v8js.flags", NULL, ZEND_INI_ALL, v8js_OnUpdateV8Flags)
ZEND_INI_END()
/* }}} */

/* }}} INI */

/* {{{ Class Entries */
zend_class_entry *php_ce_v8_object;
zend_class_entry *php_ce_v8_function;
static zend_class_entry *php_ce_v8js;
static zend_class_entry *php_ce_v8js_exception;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_object_handlers;
static zend_object_handlers v8_object_handlers;
/* }}} */

/* {{{ Extension container */
struct php_v8js_jsext {
	zend_bool auto_enable;
	HashTable *deps_ht;
	const char **deps;
	int deps_count;
	char *name;
	char *source;
};
/* }}} */

/* {{{ Context container */
struct php_v8js_ctx {
	zend_object std;
	v8::Persistent<v8::String> object_name;
	v8::Persistent<v8::Context> context;
	zend_bool report_uncaught;
	zval *pending_exception;
	int in_execution;
};
/* }}} */

/* {{{ Object container */
struct php_v8js_object {
	zend_object std;
	v8::Persistent<v8::Value> v8obj;
	int flags;
};
/* }}} */

#ifdef COMPILE_DL_V8JS
ZEND_GET_MODULE(v8js)
#endif

/* {{{ Class: V8 */

#define V8JS_V8_INVOKE_FUNC_NAME "V8Js::V8::Invoke"

/* V8 Object handlers */

static zval *php_v8js_v8_read_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL;
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (Z_TYPE_P(member) == IS_STRING && obj->v8obj->IsObject() && !obj->v8obj->IsFunction())
	{
		v8::Local<v8::Object> jsObj = obj->v8obj->ToObject();
		v8::Local<v8::String> jsKey = V8JS_STRL(Z_STRVAL_P(member), Z_STRLEN_P(member));
		v8::Local<v8::Value> jsVal;

		/* Skip any prototype properties */
		if (jsObj->HasRealNamedProperty(jsKey) || jsObj->HasRealNamedCallbackProperty(jsKey)) {
			jsVal = jsObj->Get(jsKey);
			
			if (jsVal->IsObject()) {
				ALLOC_INIT_ZVAL(retval);
				Z_SET_REFCOUNT_P(retval, 0);
			} else {
				MAKE_STD_ZVAL(retval);
			}

			if (v8js_to_zval(jsVal, retval, obj->flags TSRMLS_CC) == SUCCESS) {
				return retval;
			}
		}
	}

	ALLOC_INIT_ZVAL(retval);

	return retval;
}
/* }}} */

static void php_v8js_v8_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (obj->v8obj->IsObject() && !obj->v8obj->IsFunction()) {
		obj->v8obj->ToObject()->ForceSet(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)), zval_to_v8js(value TSRMLS_CC));
	}
}
/* }}} */

static void php_v8js_v8_unset_property(zval *object, zval *member, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (obj->v8obj->IsObject() && !obj->v8obj->IsFunction()) {
		obj->v8obj->ToObject()->ForceDelete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));
	}
}
/* }}} */

int php_v8js_v8_get_properties_hash(v8::Handle<v8::Value> jsValue, HashTable *retval, int flags TSRMLS_DC) /* {{{ */
{
	v8::Local<v8::Object> jsObj = jsValue->ToObject();

	if (!jsObj.IsEmpty()) {
		v8::Local<v8::Array> jsKeys = jsObj->GetPropertyNames();

		for (unsigned i = 0; i < jsKeys->Length(); i++)
		{
			v8::Local<v8::String> jsKey = jsKeys->Get(i)->ToString();

			/* Skip any prototype properties */
			if (!jsObj->HasRealNamedProperty(jsKey) && !jsObj->HasRealNamedCallbackProperty(jsKey) && !jsObj->HasRealIndexedProperty(i)) {
				continue;
			}

			v8::Local<v8::Value> jsVal = jsObj->Get(jsKey);
			v8::String::Utf8Value cstr(jsKey);
			const char *key = ToCString(cstr);
			zval *value = NULL;

			MAKE_STD_ZVAL(value);

			if (v8js_to_zval(jsVal, value, flags TSRMLS_CC) == FAILURE) {
				zval_ptr_dtor(&value);
				return FAILURE;
			}

			if ((flags & V8JS_FLAG_FORCE_ARRAY) || jsValue->IsArray()) {
				zend_symtable_update(retval, key, strlen(key) + 1, (void *)&value, sizeof(zval *), NULL);
			} else {
				zend_hash_update(retval, key, strlen(key) + 1, (void *) &value, sizeof(zval *), NULL);
			}
		}
		return SUCCESS;
	}
	return FAILURE;
}
/* }}} */

static HashTable *php_v8js_v8_get_properties(zval *object TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);
	HashTable *retval;
	
	ALLOC_HASHTABLE(retval);
	zend_hash_init(retval, 0, NULL, ZVAL_PTR_DTOR, 0);

	v8::HandleScope local_scope;
	v8::Handle<v8::Context> temp_context = v8::Context::New();
	v8::Context::Scope temp_scope(temp_context);

	if (php_v8js_v8_get_properties_hash(obj->v8obj, retval, obj->flags TSRMLS_CC) == SUCCESS) {
		return retval;
	}
	return NULL;
}
/* }}} */

static HashTable *php_v8js_v8_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	*is_temp = 1;
	return php_v8js_v8_get_properties(object TSRMLS_CC);
}
/* }}} */

static zend_function *php_v8js_v8_get_method(zval **object_ptr, char *method, int method_len, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_function *f;
	v8::Local<v8::String> jsKey = V8JS_STRL(method, method_len);
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(*object_ptr TSRMLS_CC);

	if (!obj->v8obj.IsEmpty() && obj->v8obj->IsObject() && !obj->v8obj->IsFunction()) {
		v8::Local<v8::Object> jsObj = obj->v8obj->ToObject();
		
		if (jsObj->Has(jsKey) && jsObj->Get(jsKey)->IsFunction()) {
			f = (zend_function *) ecalloc(1, sizeof(*f));
			f->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
			f->common.function_name = estrndup(method, method_len);
			return f;
		}
	}

	return NULL;
}
/* }}} */

static int php_v8js_v8_call_method(const char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zval *object = this_ptr, ***argv = NULL;
	int i = 0, argc = ZEND_NUM_ARGS();
	php_v8js_object *obj;

	obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (obj->v8obj.IsEmpty()) {
		zval_ptr_dtor(&object);
		return FAILURE;
	}

	if (argc > 0) {
		argv = (zval***)safe_emalloc(sizeof(zval**), argc, 0);
		zend_get_parameters_array_ex(argc, argv);
	}

	v8::Local<v8::String> method_name = V8JS_SYML(method, strlen(method));
	v8::Local<v8::Object> v8obj = obj->v8obj->ToObject();
	v8::Local<v8::Function> cb;

	if (method_name->Equals(V8JS_SYM(V8JS_V8_INVOKE_FUNC_NAME))) {
		cb = v8::Local<v8::Function>::Cast(v8obj);
	} else {
		cb = v8::Local<v8::Function>::Cast(v8obj->Get(method_name));
	}

	v8::Local<v8::Value> *jsArgv = new v8::Local<v8::Value>[argc];
	v8::Local<v8::Value> js_retval;

	for (i = 0; i < argc; i++) {
		jsArgv[i] = v8::Local<v8::Value>::New(zval_to_v8js(*argv[i] TSRMLS_CC));
	}

	js_retval = cb->Call(V8JS_GLOBAL, argc, jsArgv);

	zval_ptr_dtor(&object);

	if (argc > 0) {
		efree(argv);
	}

	if (return_value_used) {
		return v8js_to_zval(js_retval, return_value, obj->flags TSRMLS_CC);
	}

	return SUCCESS;
}
/* }}} */

static int php_v8js_v8_get_closure(zval *object, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zval **zobj_ptr TSRMLS_DC) /* {{{ */
{
	zend_function *invoke;

	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->v8obj->IsFunction()) {
		return FAILURE;
	}

	invoke = (zend_function *) ecalloc(1, sizeof(*invoke));
	invoke->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
	invoke->common.function_name = estrndup(V8JS_V8_INVOKE_FUNC_NAME, sizeof(V8JS_V8_INVOKE_FUNC_NAME) - 1);

	*fptr_ptr = invoke;

	if (zobj_ptr) {
		*zobj_ptr = object;
	}

	*ce_ptr = NULL;

	return SUCCESS;
}
/* }}} */

static void php_v8js_v8_free_storage(void *object, zend_object_handle handle TSRMLS_DC) /* {{{ */
{
	php_v8js_object *c = (php_v8js_object *) object;

	zend_object_std_dtor(&c->std TSRMLS_CC);

	if (!c->v8obj.IsEmpty()) {
		c->v8obj.Dispose();
	}

	efree(object);
}
/* }}} */

static zend_object_value php_v8js_v8_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	php_v8js_object *c;
	
	c = (php_v8js_object *) ecalloc(1, sizeof(*c));

	zend_object_std_init(&c->std, ce TSRMLS_CC);

	retval.handle = zend_objects_store_put(c, NULL, (zend_objects_free_object_storage_t) php_v8js_v8_free_storage, NULL TSRMLS_CC);
	retval.handlers = &v8_object_handlers;

	return retval;
}
/* }}} */

void php_v8js_create_v8(zval *res, v8::Handle<v8::Value> value, int flags TSRMLS_DC) /* {{{ */
{
	php_v8js_object *c;

	object_init_ex(res, value->IsFunction() ? php_ce_v8_function : php_ce_v8_object);

	c = (php_v8js_object *) zend_object_store_get_object(res TSRMLS_CC);

	c->v8obj = v8::Persistent<v8::Value>::New(value);
	c->flags = flags;
}
/* }}} */

/* }}} V8 */

/* {{{ Class: V8Js */

static void php_v8js_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	php_v8js_ctx *c = (php_v8js_ctx *) object;

	zend_object_std_dtor(&c->std TSRMLS_CC);

	if (c->pending_exception) {
		zval_ptr_dtor(&c->pending_exception);
	}
	
	c->object_name.Dispose();

	/* Clear global object, dispose context */
	if (!c->context.IsEmpty()) {
		c->context.Dispose();
		c->context.Clear();
		V8JSG(disposed_contexts) = v8::V8::ContextDisposedNotification();
#if V8JS_DEBUG
		fprintf(stderr, "Context dispose notification sent (%d)\n", V8JSG(disposed_contexts));
		fflush(stderr);
#endif
	}

	efree(object);
}
/* }}} */

static zend_object_value php_v8js_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	php_v8js_ctx *c;

	c = (php_v8js_ctx *) ecalloc(1, sizeof(*c));
	zend_object_std_init(&c->std, ce TSRMLS_CC);

	retval.handle = zend_objects_store_put(c, NULL, (zend_objects_free_object_storage_t) php_v8js_free_storage, NULL TSRMLS_CC);
	retval.handlers = &v8js_object_handlers;

	return retval;
}
/* }}} */

static void _php_v8js_free_ext_strarr(const char **arr, int count) /* {{{ */
{
	int i;

	if (arr) {
		for (i = 0; i < count; i++) {
			if (arr[i]) {
				free((void *) arr[i]);
			}
		}
		free(arr);
	}
}
/* }}} */

static void php_v8js_jsext_dtor(php_v8js_jsext *jsext) /* {{{ */
{
	if (jsext->deps_ht) {
		zend_hash_destroy(jsext->deps_ht);
		free(jsext->deps_ht);
	}
	if (jsext->deps) {
		_php_v8js_free_ext_strarr(jsext->deps, jsext->deps_count);
	}
	free(jsext->name);
	free(jsext->source);
}
/* }}} */

static int _php_v8js_create_ext_strarr(const char ***retval, int count, HashTable *ht) /* {{{ */
{
	const char **exts = NULL;
	HashPosition pos;
	zval **tmp;
	int i = 0;

	exts = (const char **) calloc(1, count * sizeof(char *));
	zend_hash_internal_pointer_reset_ex(ht, &pos);
	while (zend_hash_get_current_data_ex(ht, (void **) &tmp, &pos) == SUCCESS) {
		if (Z_TYPE_PP(tmp) == IS_STRING) {
			exts[i++] = zend_strndup(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
		} else {
			_php_v8js_free_ext_strarr(exts, i);
			return FAILURE;
		}
		zend_hash_move_forward_ex(ht, &pos);
	}
	*retval = exts;

	return SUCCESS;
}
/* }}} */

static void php_v8js_fatal_error_handler(const char *location, const char *message) /* {{{ */
{
	if (location) {
		zend_error(E_ERROR, "%s %s", location, message);
	} else {
		zend_error(E_ERROR, "%s", message);
	}
}
/* }}} */

static void php_v8js_init(TSRMLS_D) /* {{{ */
{
	/* Run only once */
	if (V8JSG(v8_initialized)) {
		return;
	}

	/* Set V8 command line flags (must be done before V8::Initialize()!) */
	if (V8JSG(v8_flags)) {
		v8::V8::SetFlagsFromString(V8JSG(v8_flags), strlen(V8JSG(v8_flags)));
	}

	/* Initialize V8 */
	v8::V8::Initialize();

	/* Redirect fatal errors to PHP error handler */
	v8::V8::SetFatalErrorHandler(php_v8js_fatal_error_handler);

	/* Run only once */
	V8JSG(v8_initialized) = 1;
}
/* }}} */

/* {{{ proto void V8Js::__construct([string object_name [, array variables [, array extensions [, bool report_uncaught_exceptions]]])
   __construct for V8Js */
static PHP_METHOD(V8Js, __construct)
{
	char *object_name = NULL;
	const char *class_name = NULL;

	int object_name_len = 0, free = 0;
	zend_uint class_name_len = 0;
	zend_bool report_uncaught = 1;
	zval *vars_arr = NULL, *exts_arr = NULL;
	const char **exts = NULL;
	int exts_count = 0;

	php_v8js_ctx *c = (php_v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!c->context.IsEmpty()) {
		/* called __construct() twice, bail out */
		return;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|saab", &object_name, &object_name_len, &vars_arr, &exts_arr, &report_uncaught) == FAILURE) {
		return;
	}

	/* Throw PHP exception if uncaught exceptions exist */
	c->report_uncaught = report_uncaught;
	c->pending_exception = NULL;
	c->in_execution = 0;

	/* Initialize V8 */
	php_v8js_init(TSRMLS_C);

	/* Include extensions used by this context */
	/* Note: Extensions registered with auto_enable do not need to be added separately like this. */
	if (exts_arr)
	{
		exts_count = zend_hash_num_elements(Z_ARRVAL_P(exts_arr));
		if (_php_v8js_create_ext_strarr(&exts, exts_count, Z_ARRVAL_P(exts_arr)) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid extensions array passed");
			return;
		}
	}

	/* Declare configuration for extensions */
	v8::ExtensionConfiguration extension_conf(exts_count, exts);

	/* Handle scope */
	v8::HandleScope handle_scope;

	/* Create global template for global object */
	if (V8JSG(global_template).IsEmpty()) {
		V8JSG(global_template) = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		V8JSG(global_template)->SetClassName(V8JS_SYM("V8Js"));

		/* Register builtin methods */
		php_v8js_register_methods(V8JSG(global_template)->InstanceTemplate());
	}

	/* Create context */
	c->context = v8::Context::New(&extension_conf, V8JSG(global_template)->InstanceTemplate());

	if (exts) {
		_php_v8js_free_ext_strarr(exts, exts_count);
	}

	/* If extensions have errors, context will be empty. (NOTE: This is V8 stuff, they expect the passed sources to compile :) */
	if (c->context.IsEmpty()) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to create V8 context. Check that registered extensions do not have errors.");
		ZVAL_NULL(getThis());
		return;
	}

	/* Enter context */
	v8::Context::Scope context_scope(c->context);

	/* Create the PHP container object's function template */
	v8::Local<v8::FunctionTemplate> php_obj_t = v8::FunctionTemplate::New();

	/* Set class name for PHP object */
	free = !zend_get_object_classname(getThis(), &class_name, &class_name_len TSRMLS_CC);
	php_obj_t->SetClassName(V8JS_SYML(class_name, class_name_len));

	if (free) {
		efree((char*)class_name);
	}
	
	/* Register Get accessor for passed variables */
	if (vars_arr && zend_hash_num_elements(Z_ARRVAL_P(vars_arr)) > 0) {
		php_v8js_register_accessors(php_obj_t->InstanceTemplate(), vars_arr TSRMLS_CC);
	}

	/* Set name for the PHP JS object */
	c->object_name = v8::Persistent<v8::String>::New((object_name_len) ? V8JS_SYML(object_name, object_name_len) : V8JS_SYM("PHP"));

	/* Add the PHP object into global object */
	V8JS_GLOBAL->Set(c->object_name, php_obj_t->InstanceTemplate()->NewInstance(), v8::ReadOnly);
}
/* }}} */

#define V8JS_BEGIN_CTX(ctx, object) \
	php_v8js_ctx *(ctx); \
	\
	if (!V8JSG(v8_initialized)) { \
		zend_error(E_ERROR, "V8 not initialized"); \
		return; \
	} \
	\
	(ctx) = (php_v8js_ctx *) zend_object_store_get_object(object TSRMLS_CC); \
	v8::Context::Scope context_scope((ctx)->context);

/* {{{ proto mixed V8Js::executeString(string script [, string identifier [, int flags]])
 */
static PHP_METHOD(V8Js, executeString)
{
	char *str = NULL, *identifier = NULL;
	int str_len = 0, identifier_len = 0;
	long flags = V8JS_FLAG_NONE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sl", &str, &str_len, &identifier, &identifier_len, &flags) == FAILURE) {
		return;
	}

	V8JS_BEGIN_CTX(c, getThis())

	/* Catch JS exceptions */
	v8::TryCatch try_catch;

	v8::HandleScope handle_scope;

	/* Set script identifier */
	v8::Local<v8::String> sname = identifier_len ? V8JS_SYML(identifier, identifier_len) : V8JS_SYM("V8Js::executeString()");

	/* Compiles a string context independently. TODO: Add a php function which calls this and returns the result as resource which can be executed later. */
	v8::Local<v8::String> source = v8::String::New(str, str_len);
	v8::Local<v8::Script> script = v8::Script::New(source, sname);

	/* Compile errors? */
	if (script.IsEmpty()) {
		php_v8js_throw_exception(&try_catch TSRMLS_CC);
		return;
	}

	/* Set flags for runtime use */
	V8JS_GLOBAL_SET_FLAGS(flags);

	/* Execute script */
	c->in_execution++;
	v8::Local<v8::Value> result = script->Run();
	c->in_execution--;

	/* Script possibly terminated, return immediately */
	if (!try_catch.CanContinue()) {
		/* TODO: throw PHP exception here? */
		return;
	}

	/* There was pending exception left from earlier executions -> throw to PHP */
	if (c->pending_exception) {
		zend_throw_exception_object(c->pending_exception TSRMLS_CC);
		c->pending_exception = NULL;
	}

	/* Handle runtime JS exceptions */
	if (try_catch.HasCaught()) {

		/* Pending exceptions are set only in outer caller, inner caller exceptions are always rethrown */
		if (c->in_execution < 1) {

			/* Report immediately if report_uncaught is true */
			if (c->report_uncaught) {
				php_v8js_throw_exception(&try_catch TSRMLS_CC);
				return;
			}

			/* Exception thrown from JS, preserve it for future execution */
			if (result.IsEmpty()) {
				MAKE_STD_ZVAL(c->pending_exception);
				php_v8js_create_exception(c->pending_exception, &try_catch TSRMLS_CC);
			}
		}

		/* Rethrow back to JS */
		try_catch.ReThrow();
		return;
	}

	/* Convert V8 value to PHP value */
	if (!result.IsEmpty()) {
		v8js_to_zval(result, return_value, flags TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto mixed V8Js::getPendingException()
 */
static PHP_METHOD(V8Js, getPendingException)
{
	php_v8js_ctx *c;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	c = (php_v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (c->pending_exception) {
		RETURN_ZVAL(c->pending_exception, 1, 0);
	}
}
/* }}} */

static void php_v8js_persistent_zval_ctor(zval **p) /* {{{ */
{
	zval *orig_ptr = *p;
	*p = (zval *) malloc(sizeof(zval));
	**p = *orig_ptr;
	switch (Z_TYPE_P(*p)) {
		case IS_STRING:
			Z_STRVAL_P(*p) = (char *) zend_strndup(Z_STRVAL_P(*p), Z_STRLEN_P(*p));
			break;
		default:
			zend_bailout();
	}
	INIT_PZVAL(*p);
}
/* }}} */

static void php_v8js_persistent_zval_dtor(zval **p) /* {{{ */
{
	switch (Z_TYPE_P(*p)) {
		case IS_STRING:
			free(Z_STRVAL_P(*p));
			break;
		default:
			zend_bailout();
	}
	free(*p);
}
/* }}} */

static int php_v8js_register_extension(char *name, uint name_len, char *source, uint source_len, zval *deps_arr, zend_bool auto_enable TSRMLS_DC) /* {{{ */
{
	php_v8js_jsext *jsext = NULL;

	if (!V8JSG(extensions)) {
		V8JSG(extensions) = (HashTable *) malloc(sizeof(HashTable));
		zend_hash_init(V8JSG(extensions), 1, NULL, (dtor_func_t) php_v8js_jsext_dtor, 1);
	} else if (zend_hash_exists(V8JSG(extensions), name, name_len + 1)) {
		return FAILURE;
	}

	jsext = (php_v8js_jsext *) calloc(1, sizeof(php_v8js_jsext));

	if (deps_arr) {
		jsext->deps_count = zend_hash_num_elements(Z_ARRVAL_P(deps_arr));

		if (_php_v8js_create_ext_strarr(&jsext->deps, jsext->deps_count, Z_ARRVAL_P(deps_arr)) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid dependancy array passed");
			php_v8js_jsext_dtor(jsext);
			free(jsext);
			return FAILURE;
		}
	}

	jsext->auto_enable = auto_enable;
	jsext->name = zend_strndup(name, name_len);
	jsext->source = zend_strndup(source, source_len);

	if (jsext->deps) {
		jsext->deps_ht = (HashTable *) malloc(sizeof(HashTable));
		zend_hash_init(jsext->deps_ht, jsext->deps_count, NULL, (dtor_func_t) php_v8js_persistent_zval_dtor, 1);
		zend_hash_copy(jsext->deps_ht, Z_ARRVAL_P(deps_arr), (copy_ctor_func_t) php_v8js_persistent_zval_ctor, NULL, sizeof(zval *));
	}

	if (zend_hash_add(V8JSG(extensions), name, name_len + 1, jsext, sizeof(php_v8js_jsext), NULL) == FAILURE) {
		php_v8js_jsext_dtor(jsext);
		free(jsext);
		return FAILURE;
	}

	v8::Extension *extension = new v8::Extension(jsext->name, jsext->source, jsext->deps_count, jsext->deps);
	extension->set_auto_enable(auto_enable ? true : false);
	v8::RegisterExtension(extension);

	free(jsext);

	return SUCCESS;
}
/* }}} */

/* {{{ proto bool V8Js::registerExtension(string ext_name, string script [, array deps [, bool auto_enable]])
 */
static PHP_METHOD(V8Js, registerExtension)
{
	char *ext_name, *script;
	zval *deps_arr = NULL;
	int ext_name_len, script_len;
	zend_bool auto_enable = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|ab", &ext_name, &ext_name_len, &script, &script_len, &deps_arr, &auto_enable) == FAILURE) {
		return;
	}

	if (!ext_name_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Extension name cannot be empty");
	} else if (!script_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Script cannot be empty");
	} else if (php_v8js_register_extension(ext_name, ext_name_len, script, script_len, deps_arr, auto_enable TSRMLS_CC) == SUCCESS) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* ## Static methods ## */

/* {{{ proto array V8Js::getExtensions()
 */
static PHP_METHOD(V8Js, getExtensions)
{
	php_v8js_jsext *jsext;
	zval *ext, *deps_arr;
	HashPosition pos;
	ulong index;
	char *key;
	uint key_len;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	if (V8JSG(extensions)) {
		zend_hash_internal_pointer_reset_ex(V8JSG(extensions), &pos);
		while (zend_hash_get_current_data_ex(V8JSG(extensions), (void **) &jsext, &pos) == SUCCESS) {
			if (zend_hash_get_current_key_ex(V8JSG(extensions), &key, &key_len, &index, 0, &pos) == HASH_KEY_IS_STRING) {
				MAKE_STD_ZVAL(ext)
				array_init(ext);
				add_assoc_bool_ex(ext, ZEND_STRS("auto_enable"), jsext->auto_enable);
				if (jsext->deps_ht) {
					MAKE_STD_ZVAL(deps_arr);
					array_init(deps_arr);
					zend_hash_copy(Z_ARRVAL_P(deps_arr), jsext->deps_ht, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
					add_assoc_zval_ex(ext, ZEND_STRS("deps"), deps_arr);
				}
				add_assoc_zval_ex(return_value, key, key_len, ext);
			}
			zend_hash_move_forward_ex(V8JSG(extensions), &pos);
		}
	}
}
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_construct, 0, 0, 0)
	ZEND_ARG_INFO(0, object_name)
	ZEND_ARG_INFO(0, variables)
	ZEND_ARG_INFO(0, extensions)
	ZEND_ARG_INFO(0, report_uncaught_exceptions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_executestring, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, identifier)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_getpendingexception, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_registerextension, 0, 0, 2)
	ZEND_ARG_INFO(0, extension_name)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, dependencies)
	ZEND_ARG_INFO(0, auto_enable)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_getextensions, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8jsexception_no_args, 0)
ZEND_END_ARG_INFO()
/* }}} */

static const zend_function_entry v8js_methods[] = { /* {{{ */
	PHP_ME(V8Js,	__construct,			arginfo_v8js_construct,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Js,	executeString,			arginfo_v8js_executestring,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	getPendingException,	arginfo_v8js_getpendingexception,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	registerExtension,		arginfo_v8js_registerextension,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(V8Js,	getExtensions,			arginfo_v8js_getextensions,			ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* V8Js object handlers */

static void php_v8js_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	v8::HandleScope handle_scope;

	/* Global PHP JS object */
	v8::Local<v8::Object> jsobj = V8JS_GLOBAL->Get(c->object_name)->ToObject();

	/* Write value to PHP JS object */
	jsobj->ForceSet(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)), zval_to_v8js(value TSRMLS_CC), v8::ReadOnly);

	/* Write value to PHP object */
	std_object_handlers.write_property(object, member, value, key TSRMLS_CC);
}
/* }}} */

static void php_v8js_unset_property(zval *object, zval *member, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	v8::HandleScope handle_scope;

	/* Global PHP JS object */
	v8::Local<v8::Object> jsobj = V8JS_GLOBAL->Get(c->object_name)->ToObject();
	
	/* Delete value from PHP JS object */
	jsobj->ForceDelete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));

	/* Unset from PHP object */
	std_object_handlers.unset_property(object, member, key TSRMLS_CC);
}
/* }}} */

/* }}} V8Js */

/* {{{ Class: V8JsException */

static void php_v8js_create_exception(zval *return_value, v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	v8::Handle<v8::Message> tc_message = try_catch->Message();
	const char *filename_string, *sourceline_string;
	char *message_string;
	int linenum, message_len;

	object_init_ex(return_value, php_ce_v8js_exception);

#define PHPV8_EXPROP(type, name, value) \
	zend_update_property##type(php_ce_v8js_exception, return_value, #name, sizeof(#name) - 1, value TSRMLS_CC);

	if (tc_message.IsEmpty()) {
		message_len = spprintf(&message_string, 0, "%s", exception_string);
	}
	else
	{
		v8::String::Utf8Value filename(tc_message->GetScriptResourceName());
		filename_string = ToCString(filename);
		PHPV8_EXPROP(_string, JsFileName, filename_string);

		v8::String::Utf8Value sourceline(tc_message->GetSourceLine());
		sourceline_string = ToCString(sourceline);
		PHPV8_EXPROP(_string, JsSourceLine, sourceline_string);

		linenum = tc_message->GetLineNumber();
		PHPV8_EXPROP(_long, JsLineNumber, linenum);

		message_len = spprintf(&message_string, 0, "%s:%d: %s", filename_string, linenum, exception_string);

		v8::String::Utf8Value stacktrace(try_catch->StackTrace());
		if (stacktrace.length() > 0) {
			const char* stacktrace_string = ToCString(stacktrace);
			PHPV8_EXPROP(_string, JsTrace, stacktrace_string);
		}
	}

	PHPV8_EXPROP(_string, message, message_string);

	efree(message_string);
}
/* }}} */

static void php_v8js_throw_exception(v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	zval *zexception = NULL;

	if (try_catch->Message().IsEmpty()) {
		zend_throw_exception(php_ce_v8js_exception, (char *) exception_string, 0 TSRMLS_CC);
	} else {
		MAKE_STD_ZVAL(zexception);
		php_v8js_create_exception(zexception, try_catch TSRMLS_CC);
		zend_throw_exception_object(zexception TSRMLS_CC);
	}
}
/* }}} */

#define V8JS_EXCEPTION_METHOD(property) \
	static PHP_METHOD(V8JsException, get##property) \
	{ \
		zval *value; \
		\
		if (zend_parse_parameters_none() == FAILURE) { \
			return; \
		} \
		value = zend_read_property(php_ce_v8js_exception, getThis(), #property, sizeof(#property) - 1, 0 TSRMLS_CC); \
		*return_value = *value; \
		zval_copy_ctor(return_value); \
		INIT_PZVAL(return_value); \
	}

/* {{{ proto string V8JsException::getJsFileName()
 */
V8JS_EXCEPTION_METHOD(JsFileName);
/* }}} */

/* {{{ proto string V8JsException::getJsLineNumber()
 */
V8JS_EXCEPTION_METHOD(JsLineNumber);
/* }}} */

/* {{{ proto string V8JsException::getJsSourceLine()
 */
V8JS_EXCEPTION_METHOD(JsSourceLine);
/* }}} */

/* {{{ proto string V8JsException::getJsTrace()
 */
V8JS_EXCEPTION_METHOD(JsTrace);	
/* }}} */

static const zend_function_entry v8js_exception_methods[] = { /* {{{ */
	PHP_ME(V8JsException,	getJsFileName,		arginfo_v8jsexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsException,	getJsLineNumber,	arginfo_v8jsexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsException,	getJsSourceLine,	arginfo_v8jsexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsException,	getJsTrace,			arginfo_v8jsexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsException */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(v8js)
{
	zend_class_entry ce;

	/* V8Object Class */
	INIT_CLASS_ENTRY(ce, "V8Object", NULL);
	php_ce_v8_object = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8_object->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8_object->create_object = php_v8js_v8_new;

	/* V8Function Class */
	INIT_CLASS_ENTRY(ce, "V8Function", NULL);
	php_ce_v8_function = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8_function->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8_function->create_object = php_v8js_v8_new;

	/* V8<Object|Function> handlers */
	memcpy(&v8_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8_object_handlers.clone_obj = NULL;
	v8_object_handlers.cast_object = NULL;
	v8_object_handlers.get_constructor = NULL;
	v8_object_handlers.get_property_ptr_ptr = NULL;
//	v8_object_handlers.has_property = php_v8js_v8_has_property; // Not implemented yet
	v8_object_handlers.read_property = php_v8js_v8_read_property;
	v8_object_handlers.write_property = php_v8js_v8_write_property;
	v8_object_handlers.unset_property = php_v8js_v8_unset_property;
	v8_object_handlers.get_properties = php_v8js_v8_get_properties;
	v8_object_handlers.get_method = php_v8js_v8_get_method;
	v8_object_handlers.call_method = php_v8js_v8_call_method;
	v8_object_handlers.get_debug_info = php_v8js_v8_get_debug_info;
	v8_object_handlers.get_closure = php_v8js_v8_get_closure;

	/* V8Js Class */
	INIT_CLASS_ENTRY(ce, "V8Js", v8js_methods);
	php_ce_v8js = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8js->create_object = php_v8js_new;

	/* V8Js handlers */
	memcpy(&v8js_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8js_object_handlers.clone_obj = NULL;
	v8js_object_handlers.write_property = php_v8js_write_property;
	v8js_object_handlers.unset_property = php_v8js_unset_property;

	/* V8Js Class Constants */
	zend_declare_class_constant_string(php_ce_v8js, ZEND_STRL("V8_VERSION"),		PHP_V8_VERSION			TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("FLAG_NONE"),			V8JS_FLAG_NONE			TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("FLAG_FORCE_ARRAY"),	V8JS_FLAG_FORCE_ARRAY	TSRMLS_CC);

	/* V8JsException Class */
	INIT_CLASS_ENTRY(ce, "V8JsException", v8js_exception_methods);
	php_ce_v8js_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_ce_v8js_exception->ce_flags |= ZEND_ACC_FINAL;

	/* Add custom JS specific properties */
	zend_declare_property_null(php_ce_v8js_exception, ZEND_STRL("JsFileName"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_exception, ZEND_STRL("JsLineNumber"),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_exception, ZEND_STRL("JsSourceLine"),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_exception, ZEND_STRL("JsTrace"),			ZEND_ACC_PROTECTED TSRMLS_CC);

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

static void php_v8js_force_v8_gc(void) /* {{{ */
{
#if V8JS_DEBUG
	TSRMLS_FETCH();
	fprintf(stderr, "############ Running V8 Idle notification: disposed/max_disposed %d/%d ############\n", V8JSG(disposed_contexts), V8JSG(max_disposed_contexts));
	fflush(stderr);
#endif

	while (!v8::V8::IdleNotification()) {
#if V8JS_DEBUG
		fprintf(stderr, ".");
		fflush(stderr);
#endif
	}

#if V8JS_DEBUG
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(v8js)
{
	UNREGISTER_INI_ENTRIES();

	v8::V8::Dispose();

	if (V8JSG(extensions)) {
		zend_hash_destroy(V8JSG(extensions));
		free(V8JSG(extensions));
		V8JSG(extensions) = NULL;
	}

	if (V8JSG(v8_flags)) {
		free(V8JSG(v8_flags));
		V8JSG(v8_flags) = NULL;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(v8js)
{
#if V8JS_DEBUG
	v8::HeapStatistics stats;
	v8::V8::GetHeapStatistics(&stats);
	float used = stats.used_heap_size() / 1024.0 / 1024.0;
	float total = stats.total_heap_size() / 1024.0 / 1024.0;

	fprintf(stderr, "### RSHUTDOWN ###\n");
	fprintf(stderr, "############ Heap Used/Total %.2f/%.2f MB ############\n", used, total);
	fflush(stderr);
#endif

	/* Force V8 to do as much GC as possible */
	if (V8JSG(max_disposed_contexts) && (V8JSG(disposed_contexts) > V8JSG(max_disposed_contexts))) {
		php_v8js_force_v8_gc();
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(v8js)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "V8 Javascript Engine", "enabled");
	php_info_print_table_header(2, "V8 Engine Compiled Version", PHP_V8_VERSION);
	php_info_print_table_header(2, "V8 Engine Linked Version", v8::V8::GetVersion());
	php_info_print_table_header(2, "Version", V8JS_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(v8js)
{
	v8js_globals->extensions = NULL;
	v8js_globals->disposed_contexts = 0;
	v8js_globals->v8_initialized = 0;
	v8js_globals->v8_flags = NULL;
}
/* }}} */

/* {{{ v8js_functions[] */
static const zend_function_entry v8js_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ v8js_module_entry
 */
zend_module_entry v8js_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	"v8js",
	v8js_functions,
	PHP_MINIT(v8js),
	PHP_MSHUTDOWN(v8js),
	NULL,
	PHP_RSHUTDOWN(v8js),
	PHP_MINFO(v8js),
	V8JS_VERSION,
	PHP_MODULE_GLOBALS(v8js),
	PHP_GINIT(v8js),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
