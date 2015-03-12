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
#include "v8js_exceptions.h"
#include "v8js_v8.h"
#include "v8js_v8object_class.h"

/* {{{ Class Entries */
zend_class_entry *php_ce_v8object;
zend_class_entry *php_ce_v8function;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_v8object_handlers;
/* }}} */

#define V8JS_V8_INVOKE_FUNC_NAME "V8Js::V8::Invoke"


/* V8 Object handlers */

static int v8js_v8object_has_property(zval *object, zval *member, int has_set_exists ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	/* param has_set_exists:
	 * 0 (has) whether property exists and is not NULL  - isset()
	 * 1 (set) whether property exists and is true-ish  - empty()
	 * 2 (exists) whether property exists               - property_exists()
	 */
	int retval = false;
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return retval;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
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

static zval *v8js_v8object_read_property(zval *object, zval *member, int type ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL;
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		ALLOC_INIT_ZVAL(retval);
		return retval;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
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

static void v8js_v8object_write_property(zval *object, zval *member, zval *value ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
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

static void v8js_v8object_unset_property(zval *object, zval *member ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);

	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (v8obj->IsObject() && !v8obj->IsFunction()) {
		v8obj->ToObject()->Delete(V8JS_SYML(Z_STRVAL_P(member), Z_STRLEN_P(member)));
	}
}
/* }}} */

static HashTable *v8js_v8object_get_properties(zval *object TSRMLS_DC) /* {{{ */
{
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);
	HashTable *retval;

	if (obj->properties == NULL) {
		if (GC_G(gc_active)) {
			/* the garbage collector is running, don't create more zvals */
			return NULL;
		}

		ALLOC_HASHTABLE(obj->properties);
		zend_hash_init(obj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);

		if (!obj->ctx) {
			/* Half-constructed object, probably due to unserialize call.
			 * Just pass back properties hash so unserialize can write to
			 * it (instead of crashing the engine). */
			return obj->properties;
		}
	} else {
		zend_hash_clean(obj->properties);
	}

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return NULL;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope local_scope(isolate);
	v8::Local<v8::Context> temp_context = v8::Context::New(isolate);
	v8::Context::Scope temp_scope(temp_context);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (v8js_get_properties_hash(v8obj, obj->properties, obj->flags, isolate TSRMLS_CC) == SUCCESS) {
		return obj->properties;
	}

	return NULL;
}
/* }}} */

static HashTable *v8js_v8object_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	*is_temp = 0;
	return v8js_v8object_get_properties(object TSRMLS_CC);
}
/* }}} */

static zend_function *v8js_v8object_get_method(zval **object_ptr, char *method, int method_len ZEND_HASH_KEY_DC TSRMLS_DC) /* {{{ */
{
	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(*object_ptr TSRMLS_CC);
	zend_function *f;

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return NULL;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
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
static int v8js_v8object_call_method(const char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
#else
static int v8js_v8object_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
#endif
{
	zval *object = this_ptr, ***argv = NULL;
	int argc = ZEND_NUM_ARGS();
	v8js_v8object *obj;

	obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return FAILURE;
	}

	if (obj->v8obj.IsEmpty()) {
		zval_ptr_dtor(&object);
		return FAILURE;
	}

	if (argc > 0) {
		argv = (zval***)safe_emalloc(sizeof(zval**), argc, 0);
		zend_get_parameters_array_ex(argc, argv);
	}

	std::function< v8::Local<v8::Value>(v8::Isolate *) > v8_call = [obj, method, argc, argv TSRMLS_CC](v8::Isolate *isolate) {
		int i = 0;

		v8::Local<v8::String> method_name = V8JS_SYML(method, strlen(method));
		v8::Local<v8::Object> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj)->ToObject();
		v8::Local<v8::Function> cb;

		if (method_name->Equals(V8JS_SYM(V8JS_V8_INVOKE_FUNC_NAME))) {
			cb = v8::Local<v8::Function>::Cast(v8obj);
		} else {
			cb = v8::Local<v8::Function>::Cast(v8obj->Get(method_name));
		}

		v8::Local<v8::Value> *jsArgv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));
		v8::Local<v8::Value> js_retval;

		for (i = 0; i < argc; i++) {
			new(&jsArgv[i]) v8::Local<v8::Value>;
			jsArgv[i] = v8::Local<v8::Value>::New(isolate, zval_to_v8js(*argv[i], isolate TSRMLS_CC));
		}

		return cb->Call(V8JS_GLOBAL(isolate), argc, jsArgv);
	};

	v8js_v8_call(obj->ctx, &return_value, obj->flags, obj->ctx->time_limit, obj->ctx->memory_limit, v8_call TSRMLS_CC);
	zval_ptr_dtor(&object);

	if (argc > 0) {
		efree(argv);
	}

	return SUCCESS;
}
/* }}} */

static int v8js_v8object_get_closure(zval *object, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zval **zobj_ptr TSRMLS_DC) /* {{{ */
{
	zend_function *invoke;

	v8js_v8object *obj = (v8js_v8object *) zend_object_store_get_object(object TSRMLS_CC);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0 TSRMLS_CC);
		return FAILURE;
	}

	v8::Isolate *isolate = obj->ctx->isolate;
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

static void v8js_v8object_free_storage(void *object, zend_object_handle handle TSRMLS_DC) /* {{{ */
{
	v8js_v8object *c = (v8js_v8object *) object;

	if (c->properties) {
		zend_hash_destroy(c->properties);
		FREE_HASHTABLE(c->properties);
		c->properties = NULL;
	}

	zend_object_std_dtor(&c->std TSRMLS_CC);

	if(c->ctx) {
		c->v8obj.Reset();
		c->ctx->v8js_v8objects.remove(c);
	}

	efree(object);
}
/* }}} */

static zend_object_value v8js_v8object_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	v8js_v8object *c;
	
	c = (v8js_v8object *) ecalloc(1, sizeof(*c));

	zend_object_std_init(&c->std, ce TSRMLS_CC);
	new(&c->v8obj) v8::Persistent<v8::Value>();

	retval.handle = zend_objects_store_put(c, NULL, (zend_objects_free_object_storage_t) v8js_v8object_free_storage, NULL TSRMLS_CC);
	retval.handlers = &v8js_v8object_handlers;

	return retval;
}
/* }}} */

/* NOTE: We could also override v8js_v8object_handlers.get_constructor to throw
 * an exception when invoked, but doing so causes the half-constructed object
 * to leak -- this seems to be a PHP bug.  So we'll define magic __construct
 * methods instead. */

/* {{{ proto V8Object::__construct()
 */
PHP_METHOD(V8Object,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8 objects!", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Object::__sleep()
 */
PHP_METHOD(V8Object, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Object instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Object::__wakeup()
 */
PHP_METHOD(V8Object, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Object instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__construct()
 */
PHP_METHOD(V8Function,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8 objects!", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__sleep()
 */
PHP_METHOD(V8Function, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Function instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__wakeup()
 */
PHP_METHOD(V8Function, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Function instances", 0 TSRMLS_CC);
	RETURN_FALSE;
}
/* }}} */

void v8js_v8object_create(zval *res, v8::Handle<v8::Value> value, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8js_v8object *c;

	object_init_ex(res, value->IsFunction() ? php_ce_v8function : php_ce_v8object);

	c = (v8js_v8object *) zend_object_store_get_object(res TSRMLS_CC);

	c->v8obj.Reset(isolate, value);
	c->flags = flags;
	c->ctx = ctx;
	c->properties = NULL;

	ctx->v8js_v8objects.push_front(c);
}
/* }}} */


static const zend_function_entry v8js_v8object_methods[] = { /* {{{ */
	PHP_ME(V8Object,	__construct,			NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Object,	__sleep,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Object,	__wakeup,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

static const zend_function_entry v8js_v8function_methods[] = { /* {{{ */
	PHP_ME(V8Function,	__construct,			NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Function,	__sleep,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Function,	__wakeup,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */


PHP_MINIT_FUNCTION(v8js_v8object_class) /* {{{ */
{
	zend_class_entry ce;

	/* V8Object Class */
	INIT_CLASS_ENTRY(ce, "V8Object", v8js_v8object_methods);
	php_ce_v8object = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8object->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8object->create_object = v8js_v8object_new;

	/* V8Function Class */
	INIT_CLASS_ENTRY(ce, "V8Function", v8js_v8function_methods);
	php_ce_v8function = zend_register_internal_class(&ce TSRMLS_CC);
	php_ce_v8function->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8function->create_object = v8js_v8object_new;

	/* V8<Object|Function> handlers */
	memcpy(&v8js_v8object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8js_v8object_handlers.clone_obj = NULL;
	v8js_v8object_handlers.cast_object = NULL;
	v8js_v8object_handlers.get_property_ptr_ptr = NULL;
	v8js_v8object_handlers.has_property = v8js_v8object_has_property;
	v8js_v8object_handlers.read_property = v8js_v8object_read_property;
	v8js_v8object_handlers.write_property = v8js_v8object_write_property;
	v8js_v8object_handlers.unset_property = v8js_v8object_unset_property;
	v8js_v8object_handlers.get_properties = v8js_v8object_get_properties;
	v8js_v8object_handlers.get_method = v8js_v8object_get_method;
	v8js_v8object_handlers.call_method = v8js_v8object_call_method;
	v8js_v8object_handlers.get_debug_info = v8js_v8object_get_debug_info;
	v8js_v8object_handlers.get_closure = v8js_v8object_get_closure;

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
