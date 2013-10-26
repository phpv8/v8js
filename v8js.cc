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

#define V8JS_DEBUG 0
//#define PHP_V8_VERSION "0.1.4"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <v8-debug.h>

#include "php_v8js_macros.h"

extern "C" {
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_smart_str.h"
#include "zend_exceptions.h"
}

/* Forward declarations */
static void php_v8js_throw_script_exception(v8::TryCatch * TSRMLS_DC);
static void php_v8js_create_script_exception(zval *, v8::TryCatch * TSRMLS_DC);

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
static zend_class_entry *php_ce_v8js_script_exception;
static zend_class_entry *php_ce_v8js_time_limit_exception;
static zend_class_entry *php_ce_v8js_memory_limit_exception;
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

#ifdef ENABLE_DEBUGGER_SUPPORT
static php_v8js_ctx *v8js_debug_context;
static int v8js_debug_auto_break_mode;
#endif

#ifdef COMPILE_DL_V8JS
ZEND_GET_MODULE(v8js)
#endif

/* {{{ Class: V8 */

#define V8JS_V8_INVOKE_FUNC_NAME "V8Js::V8::Invoke"

/* V8 Object handlers */

static int php_v8js_v8_has_property(zval *object, zval *member, int has_set_exists ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	/* param has_set_exists:
	 * 0 (has) whether property exists and is not NULL  - isset()
	 * 1 (set) whether property exists and is true-ish  - empty()
	 * 2 (exists) whether property exists               - property_exists()
	 */
	int retval = false;
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (Z_TYPE_P(member) == IS_STRING && v8obj->IsObject() && !v8obj->IsFunction())
	{

		v8::Local<v8::Object> jsObj = v8obj->ToObject();
		v8::Local<v8::String> jsKey = V8JS_STRL(Z_STRVAL_P(member), Z_STRLEN_P(member));
		v8::Local<v8::Value> jsVal;

		/* Skip any prototype properties */
		if (jsObj->HasRealNamedProperty(jsKey) || jsObj->HasRealNamedCallbackProperty(jsKey)) {
			if (has_set_exists == 2) {
				/* property_exists(), that's enough! */
				retval = true;
			} else {
				/* We need to look at the value. */
				jsVal = jsObj->Get(jsKey);
				if (has_set_exists == 0 ) {
					/* isset(): We make 'undefined' equivalent to 'null' */
					retval = !( jsVal->IsNull() || jsVal->IsUndefined() );
				} else {
					/* empty() */
					retval = jsVal->BooleanValue();
					/* for PHP compatibility, [] should also be empty */
					if (jsVal->IsArray() && retval) {
						v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(jsVal);
						retval = (array->Length() != 0);
					}
					/* for PHP compatibility, '0' should also be empty */
					if (jsVal->IsString() && retval) {
						v8::Local<v8::String> str = jsVal->ToString();
						if (str->Length() == 1) {
							uint16_t c = 0;
							str->Write(&c, 0, 1);
							if (c == '0') {
								retval = false;
							}
						}
					}
				}
			}
		}
	}
	return retval;
}
/* }}} */

static zval *php_v8js_v8_read_property(zval *object, zval *member, int type ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL;
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (Z_TYPE_P(member) == IS_STRING && v8obj->IsObject() && !v8obj->IsFunction())
	{

		v8::Local<v8::Object> jsObj = v8obj->ToObject();
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

			if (v8js_to_zval(jsVal, retval, obj->flags, isolate TSRMLS_CC) == SUCCESS) {
				return retval;
			}
		}
	}

	ALLOC_INIT_ZVAL(retval);

	return retval;
}
/* }}} */

static void php_v8js_v8_write_property(zval *object, zval *member, zval *value ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (v8obj->IsObject() && !v8obj->IsFunction()) {
		v8obj->ToObject()->ForceSet(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)), zval_to_v8js(value, isolate TSRMLS_CC));
	}
}
/* }}} */

static void php_v8js_v8_unset_property(zval *object, zval *member ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (v8obj->IsObject() && !v8obj->IsFunction()) {
		v8obj->ToObject()->ForceDelete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));
	}
}
/* }}} */

int php_v8js_v8_get_properties_hash(v8::Handle<v8::Value> jsValue, HashTable *retval, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
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

			v8::Local<v8::Value> php_object;
			if (jsVal->IsObject()) {
				php_object = v8::Local<v8::Object>::Cast(jsVal)->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY));
			}
			if (!php_object.IsEmpty()) {
				/* This is a PHP object, passed to JS and back. */
				value = reinterpret_cast<zval *>(v8::External::Cast(*php_object)->Value());
				Z_ADDREF_P(value);
			}
			else {
				MAKE_STD_ZVAL(value);

				if (v8js_to_zval(jsVal, value, flags, isolate TSRMLS_CC) == FAILURE) {
					zval_ptr_dtor(&value);
					return FAILURE;
				}
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

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (php_v8js_v8_get_properties_hash(v8obj, retval, obj->flags, isolate TSRMLS_CC) == SUCCESS) {
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

static zend_function *php_v8js_v8_get_method(zval **object_ptr, char *method, int method_len ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(*object_ptr TSRMLS_CC);
	zend_function *f;

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);
	v8::Local<v8::String> jsKey = V8JS_STRL(method, method_len);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (!obj->v8obj.IsEmpty() && v8obj->IsObject() && !v8obj->IsFunction()) {
		v8::Local<v8::Object> jsObj = v8obj->ToObject();
		
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

#if PHP_VERSION_ID >= 50400
static int php_v8js_v8_call_method(const char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
#else
static int php_v8js_v8_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
#endif
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

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::String> method_name = V8JS_SYML(method, strlen(method));
	v8::Local<v8::Object> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj)->ToObject();
	v8::Local<v8::Function> cb;

	if (method_name->Equals(V8JS_SYM(V8JS_V8_INVOKE_FUNC_NAME))) {
		cb = v8::Local<v8::Function>::Cast(v8obj);
	} else {
		cb = v8::Local<v8::Function>::Cast(v8obj->Get(method_name));
	}

	v8::Local<v8::Value> jsArgv[argc];
	v8::Local<v8::Value> js_retval;

	for (i = 0; i < argc; i++) {
		jsArgv[i] = v8::Local<v8::Value>::New(isolate, zval_to_v8js(*argv[i], isolate TSRMLS_CC));
	}

	js_retval = cb->Call(V8JS_GLOBAL, argc, jsArgv);

	zval_ptr_dtor(&object);

	if (argc > 0) {
		efree(argv);
	}

	if (return_value_used) {
		return v8js_to_zval(js_retval, return_value, obj->flags, isolate TSRMLS_CC);
	}

	return SUCCESS;
}
/* }}} */

static int php_v8js_v8_get_closure(zval *object, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zval **zobj_ptr TSRMLS_DC) /* {{{ */
{
	zend_function *invoke;

	php_v8js_object *obj = (php_v8js_object *) zend_object_store_get_object(object TSRMLS_CC);

	v8::Isolate *isolate = obj->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (!v8obj->IsFunction()) {
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

	c->v8obj.Reset();

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

void php_v8js_create_v8(zval *res, v8::Handle<v8::Value> value, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	php_v8js_object *c;

	object_init_ex(res, value->IsFunction() ? php_ce_v8_function : php_ce_v8_object);

	c = (php_v8js_object *) zend_object_store_get_object(res TSRMLS_CC);

	c->v8obj.Reset(isolate, value);
	c->flags = flags;
	c->isolate = isolate;
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
	
	c->object_name.Reset();
	c->object_name.~Persistent();
	c->global_template.Reset();
	c->global_template.~Persistent();

	/* Clear persistent handles in template cache */
	for (std::map<const char *,v8js_tmpl_t>::iterator it = c->template_cache.begin();
		 it != c->template_cache.end(); ++it) {
		it->second.Reset();
	}
	c->template_cache.~map();

	/* Clear global object, dispose context */
	if (!c->context.IsEmpty()) {
		c->context.Reset();
		V8JSG(disposed_contexts) = v8::V8::ContextDisposedNotification();
#if V8JS_DEBUG
		fprintf(stderr, "Context dispose notification sent (%d)\n", V8JSG(disposed_contexts));
		fflush(stderr);
#endif
	}
	c->context.~Persistent();

	c->modules_stack.~vector();
	c->modules_base.~vector();
	efree(object);
}
/* }}} */

static zend_object_value php_v8js_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	php_v8js_ctx *c;

	c = (php_v8js_ctx *) ecalloc(1, sizeof(*c));
	zend_object_std_init(&c->std, ce TSRMLS_CC);
	TSRMLS_SET_CTX(c->zts_ctx);

#if PHP_VERSION_ID >= 50400
	object_properties_init(&c->std, ce);
#else
	zval *tmp;
	zend_hash_copy(c->std.properties, &ce->default_properties,
				   (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#endif

	new(&c->object_name) v8::Persistent<v8::String>();
	new(&c->context) v8::Persistent<v8::Context>();
	new(&c->global_template) v8::Persistent<v8::FunctionTemplate>();

	new(&c->modules_stack) std::vector<char*>();
	new(&c->modules_base) std::vector<char*>();
	new(&c->template_cache) std::map<const char *,v8js_tmpl_t>();

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

#ifdef ENABLE_DEBUGGER_SUPPORT
static void DispatchDebugMessages() { /* {{{ */
	if(v8js_debug_context == NULL) {
		return;
	}

	v8::Isolate* isolate = v8js_debug_context->isolate;
	v8::Isolate::Scope isolate_scope(isolate);

	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context =
		v8::Local<v8::Context>::New(isolate, v8js_debug_context->context);
	v8::Context::Scope scope(context);

	v8::Debug::ProcessDebugMessages();
}
/* }}} */
#endif  /* ENABLE_DEBUGGER_SUPPORT */

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

	/* Run only once */
	V8JSG(v8_initialized) = 1;
}
/* }}} */

/* {{{ proto void V8Js::__construct([string object_name [, array variables [, array extensions [, bool report_uncaught_exceptions]]])
   __construct for V8Js */
static PHP_METHOD(V8Js, __construct)
{
	char *object_name = NULL, *class_name = NULL;
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

	/* Initialize V8 */
	php_v8js_init(TSRMLS_C);

	/* Throw PHP exception if uncaught exceptions exist */
	c->report_uncaught = report_uncaught;
	c->pending_exception = NULL;
	c->in_execution = 0;
	c->isolate = v8::Isolate::New();
	c->isolate->SetData(c);
	c->time_limit_hit = false;
	c->memory_limit_hit = false;
	c->module_loader = NULL;

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

	// Isolate execution
	v8::Isolate *isolate = c->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);

	/* Handle scope */
	v8::HandleScope handle_scope(isolate);

	/* Redirect fatal errors to PHP error handler */
	// This needs to be done within the context isolate
	v8::V8::SetFatalErrorHandler(php_v8js_fatal_error_handler);

	/* Create global template for global object */
	// Now we are using multiple isolates this needs to be created for every context

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New();
	tpl->SetClassName(V8JS_SYM("V8Js"));
	c->global_template.Reset(isolate, tpl);

	/* Register builtin methods */
	php_v8js_register_methods(tpl->InstanceTemplate(), c);

	/* Create context */
	v8::Local<v8::Context> context = v8::Context::New(isolate, &extension_conf, tpl->InstanceTemplate());
	context->SetAlignedPointerInEmbedderData(1, c);
	c->context.Reset(isolate, context);

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
	v8::Context::Scope context_scope(context);

	/* Create the PHP container object's function template */
	v8::Local<v8::FunctionTemplate> php_obj_t = v8::FunctionTemplate::New();

	/* Set class name for PHP object */
#if PHP_VERSION_ID >= 50400
	free = !zend_get_object_classname(getThis(), const_cast<const char**>(&class_name), &class_name_len TSRMLS_CC);
#else
	free = !zend_get_object_classname(getThis(), &class_name, &class_name_len TSRMLS_CC);
#endif
	php_obj_t->SetClassName(V8JS_SYML(class_name, class_name_len));

	if (free) {
		efree(class_name);
	}

	/* Register Get accessor for passed variables */
	if (vars_arr && zend_hash_num_elements(Z_ARRVAL_P(vars_arr)) > 0) {
		php_v8js_register_accessors(php_obj_t->InstanceTemplate(), vars_arr, isolate TSRMLS_CC);
	}

	/* Set name for the PHP JS object */
	v8::Local<v8::String> object_name_js = (object_name_len) ? V8JS_SYML(object_name, object_name_len) : V8JS_SYM("PHP");
	c->object_name.Reset(isolate, object_name_js);

	/* Add the PHP object into global object */
	v8::Local<v8::Object> php_obj = php_obj_t->InstanceTemplate()->NewInstance();
	V8JS_GLOBAL->Set(object_name_js, php_obj, v8::ReadOnly);

	/* Export public property values */
	HashTable *properties = zend_std_get_properties(getThis() TSRMLS_CC);
	HashPosition pos;
	zval **value;
	ulong index;
	char *member;
	uint member_len;

	for(zend_hash_internal_pointer_reset_ex(properties, &pos);
		zend_hash_get_current_data_ex(properties, (void **) &value, &pos) == SUCCESS;
		zend_hash_move_forward_ex(properties, &pos)) {
		if(zend_hash_get_current_key_ex(properties, &member, &member_len, &index, 0, &pos) != HASH_KEY_IS_STRING) {
			continue;
		}

		zval zmember;
		ZVAL_STRING(&zmember, member, 0);

		zend_property_info *property_info = zend_get_property_info(c->std.ce, &zmember, 1 TSRMLS_CC);
		if(property_info && property_info->flags & ZEND_ACC_PUBLIC) {
			/* Write value to PHP JS object */
			php_obj->ForceSet(V8JS_SYML(member, member_len - 1), zval_to_v8js(*value, isolate TSRMLS_CC), v8::ReadOnly);
		}
	}


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
	v8::Isolate *isolate = (ctx)->isolate; \
	v8::Locker locker(isolate); \
	v8::Isolate::Scope isolate_scope(isolate); \
	v8::HandleScope handle_scope(isolate); \
	v8::Context::Scope context_scope(isolate, (ctx)->context);

static void php_v8js_timer_push(long time_limit, long memory_limit, php_v8js_ctx *c TSRMLS_DC)
{
	V8JSG(timer_mutex).lock();

	// Create context for this timer
	php_v8js_timer_ctx *timer_ctx = (php_v8js_timer_ctx *)emalloc(sizeof(php_v8js_timer_ctx));

	// Calculate the time point when the time limit is exceeded
	std::chrono::milliseconds duration(time_limit);
	std::chrono::time_point<std::chrono::high_resolution_clock> from = std::chrono::high_resolution_clock::now();

	// Push the timer context
	timer_ctx->time_limit = time_limit;
	timer_ctx->memory_limit = memory_limit;
	timer_ctx->time_point = from + duration;
	timer_ctx->v8js_ctx = c;
	V8JSG(timer_stack).push(timer_ctx);

	V8JSG(timer_mutex).unlock();
}

static void php_v8js_timer_pop(TSRMLS_D)
{
	V8JSG(timer_mutex).lock();

	if (V8JSG(timer_stack).size()) {
		// Free the timer context memory
		php_v8js_timer_ctx *timer_ctx = V8JSG(timer_stack).top();
		efree(timer_ctx);

		// Remove the timer context from the stack
		V8JSG(timer_stack).pop();
	}

	V8JSG(timer_mutex).unlock();
}

static void php_v8js_terminate_execution(php_v8js_ctx *c TSRMLS_DC)
{
	// Forcefully terminate the current thread of V8 execution in the isolate
	v8::V8::TerminateExecution(c->isolate);

	// Remove this timer from the stack
	php_v8js_timer_pop(TSRMLS_C);
}

static void php_v8js_timer_thread(TSRMLS_D)
{
	while (!V8JSG(timer_stop)) {
		std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
		v8::HeapStatistics hs;

		if (V8JSG(timer_stack).size()) {
			// Get the current timer context
			php_v8js_timer_ctx *timer_ctx = V8JSG(timer_stack).top();
			php_v8js_ctx *c = timer_ctx->v8js_ctx;

			// Get memory usage statistics for the isolate
			c->isolate->GetHeapStatistics(&hs);

			if (timer_ctx->time_limit > 0 && now > timer_ctx->time_point) {
				php_v8js_terminate_execution(c TSRMLS_CC);

				V8JSG(timer_mutex).lock();
				c->time_limit_hit = true;
				V8JSG(timer_mutex).unlock();
			}

			if (timer_ctx->memory_limit > 0 && hs.used_heap_size() > timer_ctx->memory_limit) {
				php_v8js_terminate_execution(c TSRMLS_CC);

				V8JSG(timer_mutex).lock();
				c->memory_limit_hit = true;
				V8JSG(timer_mutex).unlock();
			}
		}

		// Sleep for 10ms
		std::chrono::milliseconds duration(10);
		std::this_thread::sleep_for(duration);
	}
}

/* {{{ proto mixed V8Js::executeString(string script [, string identifier [, int flags]])
 */
static PHP_METHOD(V8Js, executeString)
{
	char *str = NULL, *identifier = NULL;
	int str_len = 0, identifier_len = 0;
	long flags = V8JS_FLAG_NONE, time_limit = 0, memory_limit = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|slll", &str, &str_len, &identifier, &identifier_len, &flags, &time_limit, &memory_limit) == FAILURE) {
		return;
	}

	V8JS_BEGIN_CTX(c, getThis())

	V8JSG(timer_mutex).lock();
	c->time_limit_hit = false;
	c->memory_limit_hit = false;
	V8JSG(timer_mutex).unlock();

	/* Catch JS exceptions */
	v8::TryCatch try_catch;

	/* Set script identifier */
	v8::Local<v8::String> sname = identifier_len ? V8JS_SYML(identifier, identifier_len) : V8JS_SYM("V8Js::executeString()");

	/* Compiles a string context independently. TODO: Add a php function which calls this and returns the result as resource which can be executed later. */
	v8::Local<v8::String> source = V8JS_STRL(str, str_len);
	v8::Local<v8::Script> script = v8::Script::New(source, sname);

	/* Compile errors? */
	if (script.IsEmpty()) {
		php_v8js_throw_script_exception(&try_catch TSRMLS_CC);
		return;
	}

	/* Set flags for runtime use */
	V8JS_GLOBAL_SET_FLAGS(flags);

	if (time_limit > 0 || memory_limit > 0) {
		// If timer thread is not running then start it
		if (!V8JSG(timer_thread)) {
			// If not, start timer thread
			V8JSG(timer_thread) = new std::thread(php_v8js_timer_thread TSRMLS_CC);
		}

		php_v8js_timer_push(time_limit, memory_limit, c TSRMLS_CC);
	}

#ifdef ENABLE_DEBUGGER_SUPPORT
	if(c == v8js_debug_context && v8js_debug_auto_break_mode != V8JS_DEBUG_AUTO_BREAK_NEVER) {
		v8::Debug::DebugBreak(c->isolate);

		if(v8js_debug_auto_break_mode == V8JS_DEBUG_AUTO_BREAK_ONCE) {
			/* If break-once-mode was enabled, reset flag. */
			v8js_debug_auto_break_mode = V8JS_DEBUG_AUTO_BREAK_NEVER;
		}
	}
#endif  /* ENABLE_DEBUGGER_SUPPORT */

	/* Execute script */
	c->in_execution++;
	v8::Local<v8::Value> result = script->Run();
	c->in_execution--;

	if (time_limit > 0) {
		php_v8js_timer_pop(TSRMLS_C);
	}

	char exception_string[64];

	if (c->time_limit_hit) {
		// Execution has been terminated due to time limit
		sprintf(exception_string, "Script time limit of %lu milliseconds exceeded", time_limit);
		zend_throw_exception(php_ce_v8js_time_limit_exception, exception_string, 0 TSRMLS_CC);
		return;
	}

	if (c->memory_limit_hit) {
		// Execution has been terminated due to memory limit
		sprintf(exception_string, "Script memory limit of %lu bytes exceeded", memory_limit);
		zend_throw_exception(php_ce_v8js_memory_limit_exception, exception_string, 0 TSRMLS_CC);
		return;
	}

	if (!try_catch.CanContinue()) {
		// At this point we can't re-throw the exception
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
				php_v8js_throw_script_exception(&try_catch TSRMLS_CC);
				return;
			}

			/* Exception thrown from JS, preserve it for future execution */
			if (result.IsEmpty()) {
				MAKE_STD_ZVAL(c->pending_exception);
				php_v8js_create_script_exception(c->pending_exception, &try_catch TSRMLS_CC);
				return;
			}
		}

		/* Rethrow back to JS */
		try_catch.ReThrow();
		return;
	}

	/* Convert V8 value to PHP value */
	if (!result.IsEmpty()) {
		v8js_to_zval(result, return_value, flags, c->isolate TSRMLS_CC);
	}
}
/* }}} */

#ifdef ENABLE_DEBUGGER_SUPPORT
/* {{{ proto void V8Js::__destruct()
   __destruct for V8Js */
static PHP_METHOD(V8Js, __destruct)
{
	V8JS_BEGIN_CTX(c, getThis());

	if(v8js_debug_context == c) {
		v8::Debug::DisableAgent();
		v8js_debug_context = NULL;
	}
}
/* }}} */

/* {{{ proto bool V8Js::startDebugAgent(string agent_name[, int port[, int auto_break]])
 */
static PHP_METHOD(V8Js, startDebugAgent)
{
	char *str = NULL;
	int str_len = 0;
	long port = 0, auto_break = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sll", &str, &str_len, &port, &auto_break) == FAILURE) {
		return;
	}

	if(!port) {
		port = 9222;
	}

	V8JS_BEGIN_CTX(c, getThis());

	if(v8js_debug_context == c) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Debug agent already started for this V8Js instance");
		RETURN_BOOL(0);
	}

	if(v8js_debug_context != NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Debug agent already started for a different V8Js instance");
		RETURN_BOOL(0);
	}

	v8js_debug_context = c;
	v8js_debug_auto_break_mode = auto_break;

	v8::Debug::SetDebugMessageDispatchHandler(DispatchDebugMessages, true);
	v8::Debug::EnableAgent(str_len ? str : "V8Js", port, auto_break > 0);

	if(auto_break) {
		/* v8::Debug::EnableAgent doesn't really do what we want it to do,
		   since it only breaks processing on the default isolate.
		   Hence just trigger another DebugBreak, no for our main isolate. */
		v8::Debug::DebugBreak(c->isolate);
	}

	RETURN_BOOL(1);
}
/* }}} */
#endif  /* ENABLE_DEBUGGER_SUPPORT */

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

/* {{{ proto void V8Js::setModuleLoader(string module)
 */
static PHP_METHOD(V8Js, setModuleLoader)
{
	php_v8js_ctx *c;
	zval *callable;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &callable) == FAILURE) {
		return;
	}

	c = (php_v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	c->module_loader = callable;
	Z_ADDREF_P(c->module_loader);
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
	ZEND_ARG_INFO(0, time_limit)
	ZEND_ARG_INFO(0, memory_limit)
ZEND_END_ARG_INFO()

#ifdef ENABLE_DEBUGGER_SUPPORT
ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_destruct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_startdebugagent, 0, 0, 0)
	ZEND_ARG_INFO(0, agentName)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, auto_break)
ZEND_END_ARG_INFO()
#endif  /* ENABLE_DEBUGGER_SUPPORT */

ZEND_BEGIN_ARG_INFO(arginfo_v8js_getpendingexception, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_setmoduleloader, 0, 0, 1)
	ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_registerextension, 0, 0, 2)
	ZEND_ARG_INFO(0, extension_name)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, dependencies)
	ZEND_ARG_INFO(0, auto_enable)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_getextensions, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8jsscriptexception_no_args, 0)
ZEND_END_ARG_INFO()
/* }}} */

static const zend_function_entry v8js_methods[] = { /* {{{ */
	PHP_ME(V8Js,	__construct,			arginfo_v8js_construct,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Js,	executeString,			arginfo_v8js_executestring,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	getPendingException,	arginfo_v8js_getpendingexception,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	setModuleLoader,		arginfo_v8js_setmoduleloader,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	registerExtension,		arginfo_v8js_registerextension,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(V8Js,	getExtensions,			arginfo_v8js_getextensions,			ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#ifdef ENABLE_DEBUGGER_SUPPORT
	PHP_ME(V8Js,	__destruct,				arginfo_v8js_destruct,				ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(V8Js,	startDebugAgent,		arginfo_v8js_startdebugagent,		ZEND_ACC_PUBLIC)
#endif
	{NULL, NULL, NULL}
};
/* }}} */

/* V8Js object handlers */

static void php_v8js_write_property(zval *object, zval *member, zval *value ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	/* Check whether member is public, if so, export to V8. */
	zend_property_info *property_info = zend_get_property_info(c->std.ce, member, 1 TSRMLS_CC);
	if(property_info->flags & ZEND_ACC_PUBLIC) {
		/* Global PHP JS object */
		v8::Local<v8::String> object_name_js = v8::Local<v8::String>::New(isolate, c->object_name);
		v8::Local<v8::Object> jsobj = V8JS_GLOBAL->Get(object_name_js)->ToObject();

		/* Write value to PHP JS object */
		jsobj->ForceSet(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)), zval_to_v8js(value, isolate TSRMLS_CC), v8::ReadOnly);
	}

	/* Write value to PHP object */
	std_object_handlers.write_property(object, member, value ZEND_HASH_KEY_CC TSRMLS_CC);
}
/* }}} */

static void php_v8js_unset_property(zval *object, zval *member ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	/* Global PHP JS object */
	v8::Local<v8::String> object_name_js = v8::Local<v8::String>::New(isolate, c->object_name);
	v8::Local<v8::Object> jsobj = V8JS_GLOBAL->Get(object_name_js)->ToObject();
	
	/* Delete value from PHP JS object */
	jsobj->ForceDelete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));

	/* Unset from PHP object */
	std_object_handlers.unset_property(object, member ZEND_HASH_KEY_CC TSRMLS_CC);
}
/* }}} */

/* }}} V8Js */

/* {{{ Class: V8JsScriptException */

static void php_v8js_create_script_exception(zval *return_value, v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	v8::Handle<v8::Message> tc_message = try_catch->Message();
	const char *filename_string, *sourceline_string;
	char *message_string;
	int linenum, message_len;

	object_init_ex(return_value, php_ce_v8js_script_exception);

#define PHPV8_EXPROP(type, name, value) \
	zend_update_property##type(php_ce_v8js_script_exception, return_value, #name, sizeof(#name) - 1, value TSRMLS_CC);

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

static void php_v8js_throw_script_exception(v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	zval *zexception = NULL;

	if (try_catch->Message().IsEmpty()) {
		zend_throw_exception(php_ce_v8js_script_exception, (char *) exception_string, 0 TSRMLS_CC);
	} else {
		MAKE_STD_ZVAL(zexception);
		php_v8js_create_script_exception(zexception, try_catch TSRMLS_CC);
		zend_throw_exception_object(zexception TSRMLS_CC);
	}
}
/* }}} */

#define V8JS_EXCEPTION_METHOD(property) \
	static PHP_METHOD(V8JsScriptException, get##property) \
	{ \
		zval *value; \
		\
		if (zend_parse_parameters_none() == FAILURE) { \
			return; \
		} \
		value = zend_read_property(php_ce_v8js_script_exception, getThis(), #property, sizeof(#property) - 1, 0 TSRMLS_CC); \
		*return_value = *value; \
		zval_copy_ctor(return_value); \
		INIT_PZVAL(return_value); \
	}

/* {{{ proto string V8JsEScriptxception::getJsFileName()
 */
V8JS_EXCEPTION_METHOD(JsFileName);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsLineNumber()
 */
V8JS_EXCEPTION_METHOD(JsLineNumber);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsSourceLine()
 */
V8JS_EXCEPTION_METHOD(JsSourceLine);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsTrace()
 */
V8JS_EXCEPTION_METHOD(JsTrace);	
/* }}} */

static const zend_function_entry v8js_script_exception_methods[] = { /* {{{ */
	PHP_ME(V8JsScriptException,	getJsFileName,		arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsLineNumber,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsSourceLine,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsTrace,			arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsScriptException */

/* {{{ Class: V8JsTimeLimitException */

static const zend_function_entry v8js_time_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};

/* }}} */

/* }}} V8JsTimeLimitException */

/* {{{ Class: V8JsMemoryLimitException */

static const zend_function_entry v8js_memory_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};

/* }}} */

/* }}} V8JsMemoryLimitException */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(v8js)
{
	zend_class_entry ce;

#ifdef ZTS
	std::mutex mutex;
	memcpy(&V8JSG(timer_mutex), &mutex, sizeof(V8JSG(timer_mutex)));
#endif

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
	v8_object_handlers.has_property = php_v8js_v8_has_property;
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

#ifdef ENABLE_DEBUGGER_SUPPORT
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("DEBUG_AUTO_BREAK_NEVER"),	V8JS_DEBUG_AUTO_BREAK_NEVER			TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("DEBUG_AUTO_BREAK_ONCE"),	V8JS_DEBUG_AUTO_BREAK_ONCE			TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("DEBUG_AUTO_BREAK_ALWAYS"),	V8JS_DEBUG_AUTO_BREAK_ALWAYS		TSRMLS_CC);
#endif

	/* V8JsScriptException Class */
	INIT_CLASS_ENTRY(ce, "V8JsScriptException", v8js_script_exception_methods);
	php_ce_v8js_script_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_ce_v8js_script_exception->ce_flags |= ZEND_ACC_FINAL;

	/* Add custom JS specific properties */
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsFileName"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsLineNumber"),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsSourceLine"),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsTrace"),			ZEND_ACC_PROTECTED TSRMLS_CC);

	/* V8JsTimeLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsTimeLimitException", v8js_time_limit_exception_methods);
	php_ce_v8js_time_limit_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_ce_v8js_time_limit_exception->ce_flags |= ZEND_ACC_FINAL;

	/* V8JsMemoryLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsMemoryLimitException", v8js_memory_limit_exception_methods);
	php_ce_v8js_memory_limit_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_ce_v8js_memory_limit_exception->ce_flags |= ZEND_ACC_FINAL;

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
	// If the timer thread is running then stop it
	if (V8JSG(timer_thread)) {
		V8JSG(timer_stop) = true;
		V8JSG(timer_thread)->join();
	}

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
	v8js_globals->timer_thread = NULL;
	v8js_globals->timer_stop = false;
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
