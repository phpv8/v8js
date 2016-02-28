/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
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

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}

#include "php_v8js_macros.h"
#include "v8js_v8.h"
#include "v8js_exceptions.h"
#include "v8js_v8object_class.h"
#include "v8js_object_export.h"
#include "v8js_timer.h"

#include <functional>
#include <algorithm>

#define PHP_V8JS_SCRIPT_RES_NAME "V8Js script"

/* {{{ Class Entries */
static zend_class_entry *php_ce_v8js;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_object_handlers;
/* }}} */

/* Forward declare v8js_methods, actually "static" but not possible in C++ */
extern const zend_function_entry v8js_methods[];

typedef struct _v8js_script {
	char *name;
	v8js_ctx *ctx;
	v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>> *script;
} v8js_script;

static void v8js_script_free(v8js_script *res);

int le_v8js_script;

/* {{{ Extension container */
struct v8js_jsext {
	zend_bool auto_enable;
	HashTable *deps_ht;
	const char **deps;
	int deps_count;
	char *name;
	char *source;
	v8::Extension *extension;
};
/* }}} */

#if PHP_V8_API_VERSION >= 4004010
class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
	virtual void* Allocate(size_t length) {
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}
	virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
	virtual void Free(void* data, size_t) { free(data); }
};
#endif

static void v8js_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	v8js_ctx *c = (v8js_ctx *) object;

	zend_object_std_dtor(&c->std TSRMLS_CC);

	if (c->pending_exception) {
		zval_ptr_dtor(&c->pending_exception);
	}

	if (c->module_normaliser) {
		zval_ptr_dtor(&c->module_normaliser);
	}

	if (c->module_loader) {
		zval_ptr_dtor(&c->module_loader);
	}

	/* Delete PHP global object from JavaScript */
	if (!c->context.IsEmpty()) {
		v8::Locker locker(c->isolate);
		v8::Isolate::Scope isolate_scope(c->isolate);
		v8::HandleScope handle_scope(c->isolate);
		v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(c->isolate, c->context);
		v8::Context::Scope context_scope(v8_context);
		v8::Local<v8::String> object_name_js = v8::Local<v8::String>::New(c->isolate, c->object_name);
		V8JS_GLOBAL(c->isolate)->Delete(object_name_js);
	}

	c->object_name.Reset();
	c->object_name.~Persistent();
	c->global_template.Reset();
	c->global_template.~Persistent();
	c->array_tmpl.Reset();
	c->array_tmpl.~Persistent();

	/* Clear persistent call_impl & method_tmpls templates */
	for (std::map<v8js_tmpl_t *, v8js_tmpl_t>::iterator it = c->call_impls.begin();
		 it != c->call_impls.end(); ++it) {
		// No need to free it->first, as it is stored in c->template_cache and freed below
		it->second.Reset();
	}
	c->call_impls.~map();

	for (std::map<zend_function *, v8js_tmpl_t>::iterator it = c->method_tmpls.begin();
		 it != c->method_tmpls.end(); ++it) {
		it->second.Reset();
	}
	c->method_tmpls.~map();

	/* Clear persistent handles in template cache */
	for (std::map<const char *,v8js_tmpl_t>::iterator it = c->template_cache.begin();
		 it != c->template_cache.end(); ++it) {
		it->second.Reset();
	}
	c->template_cache.~map();

	/* Clear contexts */
	for (std::vector<v8js_accessor_ctx*>::iterator it = c->accessor_list.begin();
		 it != c->accessor_list.end(); ++it) {
		v8js_accessor_ctx_dtor(*it TSRMLS_CC);
	}
	c->accessor_list.~vector();

	/* Clear global object, dispose context */
	if (!c->context.IsEmpty()) {
		c->context.Reset();
	}
	c->context.~Persistent();

	/* Dispose yet undisposed weak refs */
	for (std::map<zval *, v8js_persistent_obj_t>::iterator it = c->weak_objects.begin();
		 it != c->weak_objects.end(); ++it) {
		zval *value = it->first;
		zval_ptr_dtor(&value);
		c->isolate->AdjustAmountOfExternalAllocatedMemory(-1024);
		it->second.Reset();
	}
	c->weak_objects.~map();

	for (std::map<v8js_tmpl_t *, v8js_persistent_obj_t>::iterator it = c->weak_closures.begin();
		 it != c->weak_closures.end(); ++it) {
		v8js_tmpl_t *persist_tpl_ = it->first;
		persist_tpl_->Reset();
		delete persist_tpl_;
		it->second.Reset();
	}
	c->weak_closures.~map();

	for (std::list<v8js_v8object *>::iterator it = c->v8js_v8objects.begin();
		 it != c->v8js_v8objects.end(); it ++) {
		(*it)->v8obj.Reset();
		(*it)->ctx = NULL;
	}
	c->v8js_v8objects.~list();

	for (std::vector<v8js_script *>::iterator it = c->script_objects.begin();
		 it != c->script_objects.end(); it ++) {
		(*it)->ctx = NULL;
		(*it)->script->Reset();
	}
	c->script_objects.~vector();

	/* Clear persistent handles in module cache */
	for (std::map<char *, v8js_persistent_obj_t>::iterator it = c->modules_loaded.begin();
		 it != c->modules_loaded.end(); ++it) {
		efree(it->first);
		it->second.Reset();
	}
	c->modules_loaded.~map();

	if(c->isolate) {
		/* c->isolate is initialized by V8Js::__construct, but __wakeup calls
		 * are not fully constructed and hence this would cause a NPE. */
		c->isolate->Dispose();
	}

	if(c->tz != NULL) {
		free(c->tz);
	}

	c->modules_stack.~vector();
	c->modules_base.~vector();

#ifdef PHP_V8_USE_EXTERNAL_STARTUP_DATA
	if (c->snapshot_blob.data) {
		efree((void*)c->snapshot_blob.data);
	}
#endif

	efree(object);
}
/* }}} */

static zend_object_value v8js_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	v8js_ctx *c;

	c = (v8js_ctx *) ecalloc(1, sizeof(*c));
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
	new(&c->array_tmpl) v8::Persistent<v8::FunctionTemplate>();

	new(&c->modules_stack) std::vector<char*>();
	new(&c->modules_base) std::vector<char*>();
	new(&c->modules_loaded) std::map<char *, v8js_persistent_obj_t, cmp_str>;

	new(&c->template_cache) std::map<const char *,v8js_tmpl_t>();
	new(&c->accessor_list) std::vector<v8js_accessor_ctx *>();

	new(&c->weak_closures) std::map<v8js_tmpl_t *, v8js_persistent_obj_t>();
	new(&c->weak_objects) std::map<zval *, v8js_persistent_obj_t>();
	new(&c->call_impls) std::map<v8js_tmpl_t *, v8js_tmpl_t>();
	new(&c->method_tmpls) std::map<zend_function *, v8js_tmpl_t>();

	new(&c->v8js_v8objects) std::list<v8js_v8object *>();
	new(&c->script_objects) std::vector<v8js_script *>();

	retval.handle = zend_objects_store_put(c, NULL, (zend_objects_free_object_storage_t) v8js_free_storage, NULL TSRMLS_CC);
	retval.handlers = &v8js_object_handlers;

	return retval;
}
/* }}} */

static void v8js_free_ext_strarr(const char **arr, int count) /* {{{ */
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

static void v8js_jsext_dtor(v8js_jsext *jsext) /* {{{ */
{
	if (jsext->deps_ht) {
		zend_hash_destroy(jsext->deps_ht);
		free(jsext->deps_ht);
	}
	if (jsext->deps) {
		v8js_free_ext_strarr(jsext->deps, jsext->deps_count);
	}
	delete jsext->extension;
	free(jsext->name);
	free(jsext->source);
}
/* }}} */

static int v8js_create_ext_strarr(const char ***retval, int count, HashTable *ht) /* {{{ */
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
			v8js_free_ext_strarr(exts, i);
			return FAILURE;
		}
		zend_hash_move_forward_ex(ht, &pos);
	}
	*retval = exts;

	return SUCCESS;
}
/* }}} */

static void v8js_fatal_error_handler(const char *location, const char *message) /* {{{ */
{
	v8::Isolate *isolate = v8::Isolate::GetCurrent();
	if (isolate) {
		isolate->Exit();
	}
	if (location) {
		zend_error(E_ERROR, "%s %s", location, message);
	} else {
		zend_error(E_ERROR, "%s", message);
	}
}
/* }}} */

#define IS_MAGIC_FUNC(mname) \
	((key_len == sizeof(mname)) && \
	!strncasecmp(key, mname, key_len - 1))

/* {{{ proto void V8Js::__construct([string object_name [, array variables [, array extensions [, bool report_uncaught_exceptions [, string snapshot_blob]]]]])
   __construct for V8Js */
static PHP_METHOD(V8Js, __construct)
{
	char *object_name = NULL, *class_name = NULL, *snapshot_blob = NULL;
	int object_name_len = 0, free = 0, snapshot_blob_len = 0;
	zend_uint class_name_len = 0;
	zend_bool report_uncaught = 1;
	zval *vars_arr = NULL, *exts_arr = NULL;
	const char **exts = NULL;
	int exts_count = 0;

	v8js_ctx *c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!c->context.IsEmpty()) {
		/* called __construct() twice, bail out */
		return;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|saabs", &object_name, &object_name_len, &vars_arr, &exts_arr, &report_uncaught, &snapshot_blob, &snapshot_blob_len) == FAILURE) {
		return;
	}

	/* Initialize V8 */
	v8js_v8_init(TSRMLS_C);

	/* Throw PHP exception if uncaught exceptions exist */
	c->report_uncaught = report_uncaught;
	c->pending_exception = NULL;
	c->in_execution = 0;

#if PHP_V8_API_VERSION >= 4004044
	static ArrayBufferAllocator array_buffer_allocator;
	new (&c->create_params) v8::Isolate::CreateParams();
	c->create_params.array_buffer_allocator = &array_buffer_allocator;

	new (&c->snapshot_blob) v8::StartupData();
#ifdef PHP_V8_USE_EXTERNAL_STARTUP_DATA
	if (snapshot_blob && snapshot_blob_len) {
		c->snapshot_blob.data = snapshot_blob;
		c->snapshot_blob.raw_size = snapshot_blob_len;
		c->create_params.snapshot_blob = &c->snapshot_blob;
	}
#endif /* PHP_V8_USE_EXTERNAL_STARTUP_DATA */

	c->isolate = v8::Isolate::New(c->create_params);
#else /* PHP_V8_API_VERSION < 4004044 */
	c->isolate = v8::Isolate::New();
#endif

	c->isolate->SetData(0, c);

	c->time_limit = 0;
	c->time_limit_hit = false;
	c->memory_limit = 0;
	c->memory_limit_hit = false;

	c->module_normaliser = NULL;
	c->module_loader = NULL;

	/* Include extensions used by this context */
	/* Note: Extensions registered with auto_enable do not need to be added separately like this. */
	if (exts_arr)
	{
		exts_count = zend_hash_num_elements(Z_ARRVAL_P(exts_arr));
		if (v8js_create_ext_strarr(&exts, exts_count, Z_ARRVAL_P(exts_arr)) == FAILURE) {
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
	v8::V8::SetFatalErrorHandler(v8js_fatal_error_handler);

	/* Create global template for global object */
	// Now we are using multiple isolates this needs to be created for every context

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(c->isolate, 0);

	tpl->SetClassName(V8JS_SYM("V8Js"));
	c->global_template.Reset(isolate, tpl);

	/* Register builtin methods */
	v8js_register_methods(tpl->InstanceTemplate(), c);

	/* Create context */
	v8::Local<v8::Context> context = v8::Context::New(isolate, &extension_conf, tpl->InstanceTemplate());

	if (exts) {
		v8js_free_ext_strarr(exts, exts_count);
	}

	/* If extensions have errors, context will be empty. (NOTE: This is V8 stuff, they expect the passed sources to compile :) */
	if (context.IsEmpty()) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to create V8 context. Check that registered extensions do not have errors.");
		ZVAL_NULL(getThis());
		return;
	}

	context->SetAlignedPointerInEmbedderData(1, c);
	c->context.Reset(isolate, context);

	/* Enter context */
	v8::Context::Scope context_scope(context);

	/* Create the PHP container object's function template */
	v8::Local<v8::FunctionTemplate> php_obj_t = v8::FunctionTemplate::New(isolate, 0);

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
		v8js_register_accessors(&c->accessor_list, php_obj_t, vars_arr, isolate TSRMLS_CC);
	}

	/* Set name for the PHP JS object */
	v8::Local<v8::String> object_name_js = (object_name_len) ? V8JS_SYML(object_name, object_name_len) : V8JS_SYM("PHP");
	c->object_name.Reset(isolate, object_name_js);

	/* Add the PHP object into global object */
	v8::Local<v8::Object> php_obj = php_obj_t->InstanceTemplate()->NewInstance();
	V8JS_GLOBAL(isolate)->ForceSet(object_name_js, php_obj, v8::ReadOnly);

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
		INIT_ZVAL(zmember);
		ZVAL_STRING(&zmember, member, 0);

		zend_property_info *property_info = zend_get_property_info(c->std.ce, &zmember, 1 TSRMLS_CC);
		if(property_info && property_info->flags & ZEND_ACC_PUBLIC) {
			/* Write value to PHP JS object */
			php_obj->ForceSet(V8JS_SYML(member, member_len - 1), zval_to_v8js(*value, isolate TSRMLS_CC), v8::ReadOnly);
		}
	}

	/* Add pointer to zend object */
	php_obj->SetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY), v8::External::New(isolate, getThis()));

	/* Export public methods */
	zend_function *method_ptr;
	char *key = NULL;
	uint key_len;

	zend_hash_internal_pointer_reset_ex(&c->std.ce->function_table, &pos);
	for (;; zend_hash_move_forward_ex(&c->std.ce->function_table, &pos)) {
		if (zend_hash_get_current_key_ex(&c->std.ce->function_table, &key, &key_len, &index, 0, &pos) != HASH_KEY_IS_STRING  ||
			zend_hash_get_current_data_ex(&c->std.ce->function_table, (void **) &method_ptr, &pos) == FAILURE
			) {
			break;
		}

		if ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0) {
			/* Allow only public methods */
			continue;
		}

		if ((method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR|ZEND_ACC_CLONE)) != 0) {
			/* no __construct, __destruct(), or __clone() functions */
			continue;
		}

		/* hide (do not export) other PHP magic functions */
		if (IS_MAGIC_FUNC(ZEND_CALLSTATIC_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SLEEP_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_WAKEUP_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SET_STATE_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_GET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_UNSET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_CALL_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_INVOKE_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_TOSTRING_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_ISSET_FUNC_NAME)) {
			continue;
		}

		const zend_function_entry *fe;
		for (fe = v8js_methods; fe->fname; fe ++) {
			if (strcmp(fe->fname, method_ptr->common.function_name) == 0) {
				break;
			}
		}

		if(fe->fname) {
			/* Method belongs to \V8Js class itself, never export to V8, even if
			 * it is overriden in a derived class. */
			continue;
		}

		v8::Local<v8::String> method_name = V8JS_STR(method_ptr->common.function_name);
		v8::Local<v8::FunctionTemplate> ft;

		try {
			ft = v8::Local<v8::FunctionTemplate>::New
				(isolate, c->method_tmpls.at(method_ptr));
		}
		catch (const std::out_of_range &) {
			ft = v8::FunctionTemplate::New(isolate, v8js_php_callback,
					v8::External::New((isolate), method_ptr));
			// @fixme add/check Signature v8::Signature::New((isolate), tmpl));
			v8js_tmpl_t *persistent_ft = &c->method_tmpls[method_ptr];
			persistent_ft->Reset(isolate, ft);
		}


		php_obj->ForceSet(method_name, ft->GetFunction());
	}
}
/* }}} */

/* {{{ proto V8JS::__sleep()
 */
PHP_METHOD(V8Js, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Js instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8JS::__wakeup()
 */
PHP_METHOD(V8Js, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Js instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

static void v8js_compile_script(zval *this_ptr, const char *str, int str_len, const char *identifier, int identifier_len, v8js_script **ret TSRMLS_DC)
{
	v8js_script *res = NULL;

	V8JS_BEGIN_CTX(c, this_ptr)

	/* Catch JS exceptions */
	v8::TryCatch try_catch;

	/* Set script identifier */
	v8::Local<v8::String> sname = identifier_len ? V8JS_SYML(identifier, identifier_len) : V8JS_SYM("V8Js::compileString()");

	/* Compiles a string context independently. TODO: Add a php function which calls this and returns the result as resource which can be executed later. */
	v8::Local<v8::String> source = V8JS_STRL(str, str_len);
	v8::Local<v8::Script> script = v8::Script::Compile(source, sname);

	/* Compile errors? */
	if (script.IsEmpty()) {
		v8js_throw_script_exception(c->isolate, &try_catch TSRMLS_CC);
		return;
	}
	res = (v8js_script *)emalloc(sizeof(v8js_script));
	res->script = new v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>>(c->isolate, script);

	v8::String::Utf8Value _sname(sname);
	res->name = estrndup(ToCString(_sname), _sname.length());
	res->ctx = c;
	*ret = res;
	return;
}

static void v8js_execute_script(zval *this_ptr, v8js_script *res, long flags, long time_limit, long memory_limit, zval **return_value TSRMLS_DC)
{
	v8js_ctx *c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (res->ctx != c) {
		zend_error(E_WARNING, "Script resource from wrong V8Js object passed");
		ZVAL_BOOL(*return_value, 0);
		return;
	}

	if (!c->in_execution && time_limit == 0) {
		time_limit = c->time_limit;
	}

	if (!c->in_execution && memory_limit == 0) {
		memory_limit = c->memory_limit;
	}

	std::function< v8::Local<v8::Value>(v8::Isolate *) > v8_call = [res](v8::Isolate *isolate) {
		v8::Local<v8::Script> script = v8::Local<v8::Script>::New(isolate, *res->script);
		return script->Run();
	};

	v8js_v8_call(c, return_value, flags, time_limit, memory_limit, v8_call TSRMLS_CC);
}

/* {{{ proto mixed V8Js::executeString(string script [, string identifier [, int flags]])
 */
static PHP_METHOD(V8Js, executeString)
{
	char *str = NULL, *identifier = NULL, *tz = NULL;
	int str_len = 0, identifier_len = 0;
	long flags = V8JS_FLAG_NONE, time_limit = 0, memory_limit = 0;
	v8js_script *res = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|slll", &str, &str_len, &identifier, &identifier_len, &flags, &time_limit, &memory_limit) == FAILURE) {
		return;
	}

	v8js_compile_script(getThis(), str, str_len, identifier, identifier_len, &res TSRMLS_CC);
	if (!res) {
		RETURN_FALSE;
	}
	v8js_execute_script(getThis(), res, flags, time_limit, memory_limit, &return_value TSRMLS_CC);
	v8js_script_free(res);
	efree(res);
}
/* }}} */


/* {{{ proto mixed V8Js::compileString(string script [, string identifier])
 */
static PHP_METHOD(V8Js, compileString)
{
	char *str = NULL, *identifier = NULL;
	int str_len = 0, identifier_len = 0;
	v8js_script *res = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &str, &str_len, &identifier, &identifier_len) == FAILURE) {
		return;
	}

	v8js_compile_script(getThis(), str, str_len, identifier, identifier_len, &res TSRMLS_CC);
	if (res) {
		ZEND_REGISTER_RESOURCE(return_value, res, le_v8js_script);

		v8js_ctx *ctx;
		ctx = (v8js_ctx *) zend_object_store_get_object(this_ptr TSRMLS_CC);
		ctx->script_objects.push_back(res);
	}
	return;
}

/* }}} */

/* {{{ proto mixed V8Js::executeScript(resource script [, int flags]])
 */
static PHP_METHOD(V8Js, executeScript)
{
	long flags = V8JS_FLAG_NONE, time_limit = 0, memory_limit = 0;
	zval *zscript;
	v8js_script *res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|lll", &zscript, &flags, &time_limit, &memory_limit) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(res, v8js_script*, &zscript, -1, PHP_V8JS_SCRIPT_RES_NAME, le_v8js_script);
	ZEND_VERIFY_RESOURCE(res);

	v8js_execute_script(getThis(), res, flags, time_limit, memory_limit, &return_value TSRMLS_CC);
}
/* }}} */

/* {{{ proto mixed V8Js::checkString(string script)
 */
static PHP_METHOD(V8Js, checkString)
{
	char *str = NULL;
	int str_len = 0;
	const char *identifier = "V8Js::checkString()";
	int identifier_len = 19;

	v8js_script *res = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		return;
	}

	v8js_compile_script(getThis(), str, str_len, identifier, identifier_len, &res TSRMLS_CC);
	if (!res) {
		RETURN_FALSE;
	}

	v8js_script_free(res);
	efree(res);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed V8Js::getPendingException()
 */
static PHP_METHOD(V8Js, getPendingException)
{
	v8js_ctx *c;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (c->pending_exception) {
		RETURN_ZVAL(c->pending_exception, 1, 0);
	}
}
/* }}} */

/* {{{ proto void V8Js::clearPendingException()
 */
static PHP_METHOD(V8Js, clearPendingException)
{
	v8js_ctx *c;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (c->pending_exception) {
		zval_ptr_dtor(&c->pending_exception);
		c->pending_exception = NULL;
	}
}
/* }}} */

/* {{{ proto void V8Js::setModuleNormaliser(string base, string module_id)
 */
static PHP_METHOD(V8Js, setModuleNormaliser)
{
	v8js_ctx *c;
	zval *callable;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &callable) == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	c->module_normaliser = callable;
	Z_ADDREF_P(c->module_normaliser);
}
/* }}} */

/* {{{ proto void V8Js::setModuleLoader(string module)
 */
static PHP_METHOD(V8Js, setModuleLoader)
{
	v8js_ctx *c;
	zval *callable;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &callable) == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	c->module_loader = callable;
	Z_ADDREF_P(c->module_loader);
}
/* }}} */

/* {{{ proto void V8Js::setTimeLimit(int time_limit)
 */
static PHP_METHOD(V8Js, setTimeLimit)
{
	v8js_ctx *c;
	long time_limit = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &time_limit) == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);
	c->time_limit = time_limit;

	V8JSG(timer_mutex).lock();
	for (std::deque< v8js_timer_ctx* >::iterator it = V8JSG(timer_stack).begin();
		 it != V8JSG(timer_stack).end(); it ++) {
		if((*it)->ctx == c && !(*it)->killed) {
			(*it)->time_limit = time_limit;

			// Calculate the time point when the time limit is exceeded
			std::chrono::milliseconds duration(time_limit);
			std::chrono::time_point<std::chrono::high_resolution_clock> from = std::chrono::high_resolution_clock::now();
			(*it)->time_point = from + duration;
		}
	}
	V8JSG(timer_mutex).unlock();

	if (c->in_execution && time_limit && !V8JSG(timer_thread)) {
		/* If timer thread is not started already and we now impose a time limit
		 * finally install the timer. */
		V8JSG(timer_thread) = new std::thread(v8js_timer_thread TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto void V8Js::setMemoryLimit(int memory_limit)
 */
static PHP_METHOD(V8Js, setMemoryLimit)
{
	v8js_ctx *c;
	long memory_limit = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &memory_limit) == FAILURE) {
		return;
	}

	c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);
	c->memory_limit = memory_limit;

	V8JSG(timer_mutex).lock();
	for (std::deque< v8js_timer_ctx* >::iterator it = V8JSG(timer_stack).begin();
		 it != V8JSG(timer_stack).end(); it ++) {
		if((*it)->ctx == c && !(*it)->killed) {
			(*it)->memory_limit = memory_limit;
		}
	}
	V8JSG(timer_mutex).unlock();

	if (c->in_execution && memory_limit && !V8JSG(timer_thread)) {
		/* If timer thread is not started already and we now impose a memory limit
		 * finally install the timer. */
		V8JSG(timer_thread) = new std::thread(v8js_timer_thread TSRMLS_CC);
	}
}
/* }}} */

static void v8js_persistent_zval_ctor(zval **p) /* {{{ */
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

static void v8js_persistent_zval_dtor(zval **p) /* {{{ */
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

static void v8js_script_free(v8js_script *res)
{
	efree(res->name);
	delete res->script; // does Reset()
}

static void v8js_script_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	v8js_script *res = (v8js_script *)rsrc->ptr;
	if (res) {
		if(res->ctx) {
			std::vector<v8js_script *>::iterator it = std::find(res->ctx->script_objects.begin(), res->ctx->script_objects.end(), res);
			res->ctx->script_objects.erase(it);
		}

		v8js_script_free(res);
		efree(res);
	}
}
/* }}} */

static int v8js_register_extension(char *name, uint name_len, char *source, uint source_len, zval *deps_arr, zend_bool auto_enable TSRMLS_DC) /* {{{ */
{
	v8js_jsext *jsext = NULL;

#ifdef ZTS
	v8js_process_globals.lock.lock();
#endif

	if (!v8js_process_globals.extensions) {
		v8js_process_globals.extensions = (HashTable *) malloc(sizeof(HashTable));
		zend_hash_init(v8js_process_globals.extensions, 1, NULL, (dtor_func_t) v8js_jsext_dtor, 1);
	} else if (zend_hash_exists(v8js_process_globals.extensions, name, name_len + 1)) {
#ifdef ZTS
		v8js_process_globals.lock.unlock();
#endif
		return FAILURE;
	}

	jsext = (v8js_jsext *) calloc(1, sizeof(v8js_jsext));

	if (deps_arr) {
		jsext->deps_count = zend_hash_num_elements(Z_ARRVAL_P(deps_arr));

		if (v8js_create_ext_strarr(&jsext->deps, jsext->deps_count, Z_ARRVAL_P(deps_arr)) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid dependency array passed");
			v8js_jsext_dtor(jsext);
			free(jsext);
#ifdef ZTS
			v8js_process_globals.lock.unlock();
#endif
			return FAILURE;
		}
	}

	jsext->auto_enable = auto_enable;
	jsext->name = zend_strndup(name, name_len);
	jsext->source = zend_strndup(source, source_len);

	if (jsext->deps) {
		jsext->deps_ht = (HashTable *) malloc(sizeof(HashTable));
		zend_hash_init(jsext->deps_ht, jsext->deps_count, NULL, (dtor_func_t) v8js_persistent_zval_dtor, 1);
		zend_hash_copy(jsext->deps_ht, Z_ARRVAL_P(deps_arr), (copy_ctor_func_t) v8js_persistent_zval_ctor, NULL, sizeof(zval *));
	}

	jsext->extension = new v8::Extension(jsext->name, jsext->source, jsext->deps_count, jsext->deps);

	if (zend_hash_add(v8js_process_globals.extensions, name, name_len + 1, jsext, sizeof(v8js_jsext), NULL) == FAILURE) {
		v8js_jsext_dtor(jsext);
		free(jsext);
#ifdef ZTS
		v8js_process_globals.lock.unlock();
#endif
		return FAILURE;
	}

#ifdef ZTS
	v8js_process_globals.lock.unlock();
#endif

	jsext->extension->set_auto_enable(auto_enable ? true : false);
	v8::RegisterExtension(jsext->extension);

	free(jsext);
	return SUCCESS;
}
/* }}} */



/* ## Static methods ## */

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
	} else if (v8js_register_extension(ext_name, ext_name_len, script, script_len, deps_arr, auto_enable TSRMLS_CC) == SUCCESS) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto array V8Js::getExtensions()
 */
static PHP_METHOD(V8Js, getExtensions)
{
	v8js_jsext *jsext;
	zval *ext, *deps_arr;
	HashPosition pos;
	ulong index;
	char *key;
	uint key_len;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);

#ifdef ZTS
	v8js_process_globals.lock.lock();
#endif

	if (v8js_process_globals.extensions) {
		zend_hash_internal_pointer_reset_ex(v8js_process_globals.extensions, &pos);
		while (zend_hash_get_current_data_ex(v8js_process_globals.extensions, (void **) &jsext, &pos) == SUCCESS) {
			if (zend_hash_get_current_key_ex(v8js_process_globals.extensions, &key, &key_len, &index, 0, &pos) == HASH_KEY_IS_STRING) {
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
			zend_hash_move_forward_ex(v8js_process_globals.extensions, &pos);
		}
	}

#ifdef ZTS
	v8js_process_globals.lock.unlock();
#endif
}
/* }}} */

#ifdef PHP_V8_USE_EXTERNAL_STARTUP_DATA
/* {{{ proto string|bool V8Js::createSnapshot(string embed_source)
 */
static PHP_METHOD(V8Js, createSnapshot)
{
	char *script;
	int script_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &script, &script_len) == FAILURE) {
		return;
	}

	if (!script_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Script cannot be empty");
		RETURN_FALSE;
	}

	/* Initialize V8, if not already done. */
	v8js_v8_init(TSRMLS_C);

	v8::StartupData snapshot_blob = v8::V8::CreateSnapshotDataBlob(script);

	if (!snapshot_blob.data) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to create V8 heap snapshot.  Check $embed_source for errors.");
		RETURN_FALSE;
	}

	RETVAL_STRINGL(snapshot_blob.data, snapshot_blob.raw_size, 1);
	delete[] snapshot_blob.data;
}
/* }}} */
#endif  /* PHP_V8_USE_EXTERNAL_STARTUP_DATA */


/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_construct, 0, 0, 0)
	ZEND_ARG_INFO(0, object_name)
	ZEND_ARG_INFO(0, variables)
	ZEND_ARG_INFO(0, extensions)
	ZEND_ARG_INFO(0, report_uncaught_exceptions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_sleep, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_wakeup, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_executestring, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, identifier)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, time_limit)
	ZEND_ARG_INFO(0, memory_limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_compilestring, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, identifier)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_executescript, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, time_limit)
	ZEND_ARG_INFO(0, memory_limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_checkstring, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_getpendingexception, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8js_clearpendingexception, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_setmodulenormaliser, 0, 0, 2)
	ZEND_ARG_INFO(0, base)
	ZEND_ARG_INFO(0, module_id)
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

#ifdef PHP_V8_USE_EXTERNAL_STARTUP_DATA
ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_createsnapshot, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_settimelimit, 0, 0, 1)
	ZEND_ARG_INFO(0, time_limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8js_setmemorylimit, 0, 0, 1)
	ZEND_ARG_INFO(0, memory_limit)
ZEND_END_ARG_INFO()


const zend_function_entry v8js_methods[] = { /* {{{ */
	PHP_ME(V8Js,	__construct,			arginfo_v8js_construct,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Js,	__sleep,				arginfo_v8js_sleep,					ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Js,	__wakeup,				arginfo_v8js_sleep,					ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Js,	executeString,			arginfo_v8js_executestring,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	compileString,			arginfo_v8js_compilestring,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,    executeScript,			arginfo_v8js_executescript,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,    checkString,			arginfo_v8js_checkstring,			ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	PHP_ME(V8Js,	getPendingException,	arginfo_v8js_getpendingexception,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	clearPendingException,	arginfo_v8js_clearpendingexception,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	setModuleNormaliser,	arginfo_v8js_setmodulenormaliser,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	setModuleLoader,		arginfo_v8js_setmoduleloader,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	setTimeLimit,			arginfo_v8js_settimelimit,			ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	setMemoryLimit,			arginfo_v8js_setmemorylimit,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Js,	registerExtension,		arginfo_v8js_registerextension,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(V8Js,	getExtensions,			arginfo_v8js_getextensions,			ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#ifdef PHP_V8_USE_EXTERNAL_STARTUP_DATA
	PHP_ME(V8Js,	createSnapshot,			arginfo_v8js_createsnapshot,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#endif
	{NULL, NULL, NULL}
};
/* }}} */



/* V8Js object handlers */

static void v8js_write_property(zval *object, zval *member, zval *value ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	/* Check whether member is public, if so, export to V8. */
	zend_property_info *property_info = zend_get_property_info(c->std.ce, member, 1 TSRMLS_CC);
	if(property_info->flags & ZEND_ACC_PUBLIC) {
		/* Global PHP JS object */
		v8::Local<v8::String> object_name_js = v8::Local<v8::String>::New(isolate, c->object_name);
		v8::Local<v8::Object> jsobj = V8JS_GLOBAL(isolate)->Get(object_name_js)->ToObject();

		/* Write value to PHP JS object */
		jsobj->ForceSet(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)), zval_to_v8js(value, isolate TSRMLS_CC), v8::ReadOnly);
	}

	/* Write value to PHP object */
	std_object_handlers.write_property(object, member, value ZEND_HASH_KEY_CC TSRMLS_CC);
}
/* }}} */

static void v8js_unset_property(zval *object, zval *member ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	V8JS_BEGIN_CTX(c, object)

	/* Global PHP JS object */
	v8::Local<v8::String> object_name_js = v8::Local<v8::String>::New(isolate, c->object_name);
	v8::Local<v8::Object> jsobj = V8JS_GLOBAL(isolate)->Get(object_name_js)->ToObject();

	/* Delete value from PHP JS object */
	jsobj->Delete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));

	/* Unset from PHP object */
	std_object_handlers.unset_property(object, member ZEND_HASH_KEY_CC TSRMLS_CC);
}
/* }}} */

PHP_MINIT_FUNCTION(v8js_class) /* {{{ */
{
	zend_class_entry ce;

	/* V8Js Class */
	INIT_CLASS_ENTRY(ce, "V8Js", v8js_methods);
	php_ce_v8js = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8js->create_object = v8js_new;

	/* V8Js handlers */
	memcpy(&v8js_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8js_object_handlers.clone_obj = NULL;
	v8js_object_handlers.write_property = v8js_write_property;
	v8js_object_handlers.unset_property = v8js_unset_property;

	/* V8Js Class Constants */
	zend_declare_class_constant_string(php_ce_v8js, ZEND_STRL("V8_VERSION"),		PHP_V8_VERSION			TSRMLS_CC);

	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("FLAG_NONE"),			V8JS_FLAG_NONE			TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("FLAG_FORCE_ARRAY"),	V8JS_FLAG_FORCE_ARRAY	TSRMLS_CC);
	zend_declare_class_constant_long(php_ce_v8js, ZEND_STRL("FLAG_PROPAGATE_PHP_EXCEPTIONS"), V8JS_FLAG_PROPAGATE_PHP_EXCEPTIONS TSRMLS_CC);

	le_v8js_script = zend_register_list_destructors_ex(v8js_script_dtor, NULL, PHP_V8JS_SCRIPT_RES_NAME, module_number);

#if PHP_V8_API_VERSION >= 4004010 && PHP_V8_API_VERSION < 4004044
	static ArrayBufferAllocator array_buffer_allocator;
	v8::V8::SetArrayBufferAllocator(&array_buffer_allocator);
#endif

	return SUCCESS;
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
