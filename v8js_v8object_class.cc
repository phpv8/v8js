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
#include "v8js_exceptions.h"
#include "v8js_v8.h"
#include "v8js_v8object_class.h"

extern "C" {
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}

/* {{{ Class Entries */
zend_class_entry *php_ce_v8object;
zend_class_entry *php_ce_v8function;
zend_class_entry *php_ce_v8generator;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_v8object_handlers;
static zend_object_handlers v8js_v8generator_handlers;
/* }}} */

#define V8JS_V8_INVOKE_FUNC_NAME "V8Js::V8::Invoke"


/* V8 Object handlers */

static int v8js_v8object_has_property(zval *object, zval *member, int has_set_exists, void **cache_slot) /* {{{ */
{
	/* param has_set_exists:
	 * 0 (has) whether property exists and is not NULL  - isset()
	 * 1 (set) whether property exists and is true-ish  - empty()
	 * 2 (exists) whether property exists               - property_exists()
	 */
	int retval = false;
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return false;
	}

	V8JS_CTX_PROLOGUE_EX(obj->ctx, false);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);
	v8::Local<v8::Object> jsObj;

	if (Z_TYPE_P(member) != IS_STRING || !v8obj->IsObject() || !v8obj->ToObject(v8_context).ToLocal(&jsObj)) {
		return false;
	}

	if (Z_STRLEN_P(member) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Member name length exceeds maximum supported length", 0);
		return false;
	}

	v8::Local<v8::String> jsKey = V8JS_ZSYM(Z_STR_P(member));

	/* Skip any prototype properties */
	if (!jsObj->HasRealNamedProperty(v8_context, jsKey).FromMaybe(false)
			&& !jsObj->HasRealNamedCallbackProperty(v8_context, jsKey).FromMaybe(false)) {
		return false;
	}

	if (has_set_exists == 2) {
		/* property_exists(), that's enough! */
		return true;
	}

	/* We need to look at the value. */
	v8::Local<v8::Value> jsVal = jsObj->Get(v8_context, jsKey).ToLocalChecked();

	if (has_set_exists == 0 ) {
		/* isset(): We make 'undefined' equivalent to 'null' */
		return !( jsVal->IsNull() || jsVal->IsUndefined() );
	}

	/* empty() */
	retval = jsVal->BooleanValue(isolate);

	/* for PHP compatibility, [] should also be empty */
	if (jsVal->IsArray() && retval) {
		v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(jsVal);
		retval = (array->Length() != 0);
	}

	/* for PHP compatibility, '0' should also be empty */
	v8::Local<v8::String> str;
	if (jsVal->IsString() && retval && jsVal->ToString(v8_context).ToLocal(&str)
			&& str->Length() == 1) {
		uint16_t c = 0;
		str->Write(isolate, &c, 0, 1);
		if (c == '0') {
			retval = false;
		}
	}

	return retval;
}
/* }}} */

static zval *v8js_v8object_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv) /* {{{ */
{
	zval *retval = rv;
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return retval;
	}

	V8JS_CTX_PROLOGUE_EX(obj->ctx, retval);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (Z_TYPE_P(member) == IS_STRING && v8obj->IsObject())
	{
		if (Z_STRLEN_P(member) > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Member name length exceeds maximum supported length", 0);
			return retval;
		}

		v8::Local<v8::String> jsKey = V8JS_ZSYM(Z_STR_P(member));
		v8::Local<v8::Object> jsObj = v8obj->ToObject(v8_context).ToLocalChecked();

		/* Skip any prototype properties */
		if (jsObj->HasRealNamedProperty(v8_context, jsKey).FromMaybe(false)
				|| jsObj->HasRealNamedCallbackProperty(v8_context, jsKey).FromMaybe(false)) {
			v8::MaybeLocal<v8::Value> jsVal = jsObj->Get(v8_context, jsKey);
			
			if (!jsVal.IsEmpty() && v8js_to_zval(jsVal.ToLocalChecked(), retval, obj->flags, isolate) == SUCCESS) {
				return retval;
			}
		}
	}

	return retval;
}
/* }}} */

static void v8js_v8object_write_property(zval *object, zval *member, zval *value, void **cache_slot ) /* {{{ */
{
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return;
	}

	V8JS_CTX_PROLOGUE(obj->ctx);
	v8::Local<v8::Value> v8objHandle = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (Z_STRLEN_P(member) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Member name length exceeds maximum supported length", 0);
		return;
	}

	v8::Local<v8::Object> v8obj;
	if (v8objHandle->IsObject() && v8objHandle->ToObject(v8_context).ToLocal(&v8obj)) {
		v8obj->CreateDataProperty(v8_context, V8JS_ZSYM(Z_STR_P(member)), zval_to_v8js(value, isolate));
	}
}
/* }}} */

static void v8js_v8object_unset_property(zval *object, zval *member, void **cache_slot) /* {{{ */
{
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return;
	}

	V8JS_CTX_PROLOGUE(obj->ctx);
	v8::Local<v8::Value> v8objHandle = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (Z_STRLEN_P(member) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Member name length exceeds maximum supported length", 0);
		return;
	}

	v8::Local<v8::Object> v8obj;
	if (v8objHandle->IsObject() && v8objHandle->ToObject(v8_context).ToLocal(&v8obj)) {
		v8obj->Delete(v8_context, V8JS_ZSYM(Z_STR_P(member)));
	}
}
/* }}} */

static HashTable *v8js_v8object_get_properties(zval *object) /* {{{ */
{
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (obj->properties == NULL) {
#if PHP_VERSION_ID < 70300
		if (GC_G(gc_active)) {
			/* the garbage collector is running, don't create more zvals */
			return NULL;
		}
#endif

		ALLOC_HASHTABLE(obj->properties);
		zend_hash_init(obj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);

		if (!obj->ctx) {
			/* Half-constructed object, probably due to unserialize call.
			 * Just pass back properties hash so unserialize can write to
			 * it (instead of crashing the engine). */
			return obj->properties;
		}
	} else if (!obj->properties->u.v.nIteratorsCount) {
		zend_hash_clean(obj->properties);
	}

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return NULL;
	}

	V8JS_CTX_PROLOGUE_EX(obj->ctx, NULL);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (v8js_get_properties_hash(v8obj, obj->properties, obj->flags, isolate) == SUCCESS) {
		return obj->properties;
	}

	return NULL;
}
/* }}} */

static HashTable *v8js_v8object_get_debug_info(zval *object, int *is_temp) /* {{{ */
{
	*is_temp = 0;
	return v8js_v8object_get_properties(object);
}
/* }}} */

static zend_function *v8js_v8object_get_method(zend_object **object_ptr, zend_string *method, const zval *key) /* {{{ */
{
	v8js_v8object *obj = v8js_v8object_fetch_object(*object_ptr);
	zend_function *f;

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return NULL;
	}

	if (ZSTR_LEN(method) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Method name length exceeds maximum supported length", 0);
		return NULL;
	}

	V8JS_CTX_PROLOGUE_EX(obj->ctx, NULL);
	v8::Local<v8::String> jsKey = V8JS_STRL(ZSTR_VAL(method), static_cast<int>(ZSTR_LEN(method)));
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (!obj->v8obj.IsEmpty() && v8obj->IsObject() && !v8obj->IsFunction()) {
		v8::Local<v8::Object> jsObj;
		v8::Local<v8::Value> jsObjSlot;

		if (v8obj->ToObject(v8_context).ToLocal(&jsObj)
				&& jsObj->Has(v8_context, jsKey).FromMaybe(false)
				&& jsObj->Get(v8_context, jsKey).ToLocal(&jsObjSlot)
				&& jsObjSlot->IsFunction()) {
			f = (zend_function *) ecalloc(1, sizeof(*f));
			f->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
			f->common.function_name = zend_string_copy(method);
			return f;
		}
	}

	return NULL;
}
/* }}} */

static int v8js_v8object_call_method(zend_string *method, zend_object *object, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zval *argv = NULL;
	int argc = ZEND_NUM_ARGS();

	v8js_v8object *obj = v8js_v8object_fetch_object(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return FAILURE;
	}

	if (obj->v8obj.IsEmpty()) {
		return FAILURE;
	}

	if (ZSTR_LEN(method) > std::numeric_limits<int>::max()) {
		zend_throw_exception(php_ce_v8js_exception,
			"Method name length exceeds maximum supported length", 0);
		return FAILURE;
	}

	if (argc > 0) {
		argv = (zval*)safe_emalloc(sizeof(zval), argc, 0);
		zend_get_parameters_array_ex(argc, argv);
	}

	/* std::function relies on its dtor to be executed, otherwise it leaks
	 * some memory on bailout. */
	{
		std::function< v8::MaybeLocal<v8::Value>(v8::Isolate *) > v8_call = [obj, method, argc, argv, object, &return_value](v8::Isolate *isolate) {
			int i = 0;

			v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();
			v8::Local<v8::String> method_name = V8JS_SYML(ZSTR_VAL(method), static_cast<int>(ZSTR_LEN(method)));
			v8::Local<v8::Object> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj)->ToObject(v8_context).ToLocalChecked();
			v8::Local<v8::Object> thisObj;
			v8::Local<v8::Function> cb;

			if (method_name->Equals(v8_context, V8JS_SYM(V8JS_V8_INVOKE_FUNC_NAME)).FromMaybe(false)) {
				cb = v8::Local<v8::Function>::Cast(v8obj);
			} else {
				v8::Local<v8::Value> slot;

				if (!v8obj->Get(v8_context, method_name).ToLocal(&slot)) {
					return v8::MaybeLocal<v8::Value>();
				}

				cb = v8::Local<v8::Function>::Cast(slot);
			}

			// If a method is invoked on V8Object, then set the object itself as
			// "this" on JS side.  Otherwise fall back to global object.
			if (obj->std.ce == php_ce_v8object) {
				thisObj = v8obj;
			}
			else {
				thisObj = V8JS_GLOBAL(isolate);
			}

			v8::Local<v8::Value> *jsArgv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));

			for (i = 0; i < argc; i++) {
				new(&jsArgv[i]) v8::Local<v8::Value>;
				jsArgv[i] = v8::Local<v8::Value>::New(isolate, zval_to_v8js(&argv[i], isolate));
			}

			v8::MaybeLocal<v8::Value> result = cb->Call(v8_context, thisObj, argc, jsArgv);

			if (obj->std.ce == php_ce_v8object && !result.IsEmpty() && result.ToLocalChecked()->StrictEquals(thisObj)) {
				/* JS code did "return this", retain object identity */
				ZVAL_OBJ(return_value, object);
				zval_copy_ctor(return_value);
				result = v8::MaybeLocal<v8::Value>();
			}

			return result;
		};

		v8js_v8_call(obj->ctx, &return_value, obj->flags, obj->ctx->time_limit, obj->ctx->memory_limit, v8_call);
	}

	if (argc > 0) {
		efree(argv);
	}

	if(V8JSG(fatal_error_abort)) {
		/* Check for fatal error marker possibly set by v8js_error_handler; just
		 * rethrow the error since we're now out of V8. */
		zend_bailout();
	}

	return SUCCESS;
}
/* }}} */

static int v8js_v8object_get_closure(zval *object, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zend_object **zobj_ptr) /* {{{ */
{
	zend_function *invoke;
	v8js_v8object *obj = Z_V8JS_V8OBJECT_OBJ_P(object);

	if (!obj->ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Object after V8Js instance is destroyed!", 0);
		return FAILURE;
	}

	V8JS_CTX_PROLOGUE_EX(obj->ctx, FAILURE);
	v8::Local<v8::Value> v8obj = v8::Local<v8::Value>::New(isolate, obj->v8obj);

	if (!v8obj->IsFunction()) {
		return FAILURE;
	}

	invoke = (zend_function *) ecalloc(1, sizeof(*invoke));
	invoke->type = ZEND_OVERLOADED_FUNCTION_TEMPORARY;
	invoke->common.function_name = zend_string_init(V8JS_V8_INVOKE_FUNC_NAME, sizeof(V8JS_V8_INVOKE_FUNC_NAME) - 1, 0);

	*fptr_ptr = invoke;

	if (zobj_ptr) {
		*zobj_ptr = Z_OBJ_P(object);
	}

	*ce_ptr = NULL;

	return SUCCESS;
}
/* }}} */

static void v8js_v8object_free_storage(zend_object *object) /* {{{ */
{
	v8js_v8object *c = v8js_v8object_fetch_object(object);

	if (c->properties) {
		zend_hash_destroy(c->properties);
		FREE_HASHTABLE(c->properties);
		c->properties = NULL;
	}

	zend_object_std_dtor(&c->std);

	if(c->ctx) {
		c->v8obj.Reset();
		c->ctx->v8js_v8objects.remove(c);
	}
}
/* }}} */

static zend_object *v8js_v8object_new(zend_class_entry *ce) /* {{{ */
{
	v8js_v8object *c;
	c = (v8js_v8object *) ecalloc(1, sizeof(v8js_v8object) + zend_object_properties_size(ce));

	zend_object_std_init(&c->std, ce);
	c->std.handlers = &v8js_v8object_handlers;
	new(&c->v8obj) v8::Persistent<v8::Value>();

	return &c->std;
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
		"Can't directly construct V8 objects!", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Object::__sleep()
 */
PHP_METHOD(V8Object, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Object instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Object::__wakeup()
 */
PHP_METHOD(V8Object, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Object instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__construct()
 */
PHP_METHOD(V8Function,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8 objects!", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__sleep()
 */
PHP_METHOD(V8Function, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Function instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Function::__wakeup()
 */
PHP_METHOD(V8Function, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Function instances", 0);
	RETURN_FALSE;
}
/* }}} */


static void v8js_v8generator_free_storage(zend_object *object) /* {{{ */
{
	v8js_v8generator *c = v8js_v8generator_fetch_object(object);
	zval_ptr_dtor(&c->value);

	v8js_v8object_free_storage(object);
}
/* }}} */

static zend_object *v8js_v8generator_new(zend_class_entry *ce) /* {{{ */
{
	v8js_v8generator *c;
	c = (v8js_v8generator *) ecalloc(1, sizeof(v8js_v8generator) + zend_object_properties_size(ce));

	zend_object_std_init(&c->v8obj.std, ce);
	c->v8obj.std.handlers = &v8js_v8generator_handlers;
	new(&c->v8obj.v8obj) v8::Persistent<v8::Value>();

	return &c->v8obj.std;
}
/* }}} */

static void v8js_v8generator_next(v8js_v8generator *g) /* {{{ */
{
	if (!g->v8obj.ctx) {
		zend_throw_exception(php_ce_v8js_exception,
			"Can't access V8Generator after V8Js instance is destroyed!", 0);
		return;
	}

	/* std::function relies on its dtor to be executed, otherwise it leaks
	 * some memory on bailout. */
	{
		std::function< v8::MaybeLocal<v8::Value>(v8::Isolate *) > v8_call = [g](v8::Isolate *isolate) {
			v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();
			v8::Local<v8::String> method_name = V8JS_SYM("next");
			v8::Local<v8::Object> v8obj = v8::Local<v8::Value>::New(isolate, g->v8obj.v8obj)->ToObject(v8_context).ToLocalChecked();
			v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(v8obj->Get(v8_context, method_name).ToLocalChecked());;

			v8::MaybeLocal<v8::Value> result = cb->Call(v8_context, v8obj, 0, NULL);

			if(result.IsEmpty()) {
				/* cb->Call probably threw (and already threw a zend exception), just return */
				return V8JS_NULL;
			}

			if(!result.ToLocalChecked()->IsObject()) {
				zend_throw_exception(php_ce_v8js_exception,
					"V8Generator returned non-object on next()", 0);
				return V8JS_NULL;
			}


			v8::Local<v8::Object> resultObj = result.ToLocalChecked()->ToObject(v8_context).ToLocalChecked();
			v8::Local<v8::Value> val = resultObj->Get(v8_context, V8JS_SYM("value")).ToLocalChecked();
			v8::Local<v8::Value> done = resultObj->Get(v8_context, V8JS_SYM("done")).ToLocalChecked();

			zval_ptr_dtor(&g->value);
			v8js_to_zval(val, &g->value, 0, isolate);

			g->done = done->IsTrue();
			g->primed = true;
			return V8JS_NULL;
		};

		v8js_v8_call(g->v8obj.ctx, NULL, g->v8obj.flags, g->v8obj.ctx->time_limit, g->v8obj.ctx->memory_limit, v8_call);
	}

	if(V8JSG(fatal_error_abort)) {
		/* Check for fatal error marker possibly set by v8js_error_handler; just
		 * rethrow the error since we're now out of V8. */
		zend_bailout();
	}
}
/* }}} */

static zend_function *v8js_v8generator_get_method(zend_object **object_ptr, zend_string *method, const zval *key) /* {{{ */
{
	zend_function *result = std_object_handlers.get_method(object_ptr, method, key);

	if(!result) {
		result = v8js_v8object_get_method(object_ptr, method, key);
	}

	return result;
}
/* }}} */

/* {{{ proto V8Generator::__construct()
 */
PHP_METHOD(V8Generator,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8 objects!", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Generator::__sleep()
 */
PHP_METHOD(V8Generator, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Generator instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Generator::__wakeup()
 */
PHP_METHOD(V8Generator, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Generator instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ mixed V8Generator::current()
 */
PHP_METHOD(V8Generator, current)
{
	v8js_v8generator *g = Z_V8JS_V8GENERATOR_OBJ_P(getThis());

	if(!g->primed) {
		v8js_v8generator_next(g);
	}

	RETVAL_ZVAL(&g->value, 1, 0);
}
/* }}} */

/* {{{ scalar V8Generator::key()
 */
PHP_METHOD(V8Generator, key)
{
	RETURN_FALSE;
}
/* }}} */

/* {{{ void V8Generator::next()
 */
PHP_METHOD(V8Generator, next)
{
	v8js_v8generator *g = Z_V8JS_V8GENERATOR_OBJ_P(getThis());
	v8js_v8generator_next(g);
}
/* }}} */

/* {{{ void V8Generator::rewind()
 */
PHP_METHOD(V8Generator, rewind)
{
	v8js_v8generator *g = Z_V8JS_V8GENERATOR_OBJ_P(getThis());

	if(g->primed) {
		zend_throw_exception(php_ce_v8js_exception,
			"V8Generator::rewind not supported by ES6", 0);

	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ boolean V8Generator::valid()
 */
PHP_METHOD(V8Generator, valid)
{
	v8js_v8generator *g = Z_V8JS_V8GENERATOR_OBJ_P(getThis());

	if(!g->primed) {
		v8js_v8generator_next(g);
	}

	RETVAL_BOOL(!g->done);
}
/* }}} */


void v8js_v8object_create(zval *res, v8::Local<v8::Value> value, int flags, v8::Isolate *isolate) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	if(value->IsGeneratorObject()) {
		object_init_ex(res, php_ce_v8generator);
	}
	else if(value->IsFunction()) {
		object_init_ex(res, php_ce_v8function);
	}
	else {
		object_init_ex(res, php_ce_v8object);
	}

	v8js_v8object *c = Z_V8JS_V8OBJECT_OBJ_P(res);

	c->v8obj.Reset(isolate, value);
	c->flags = flags;
	c->ctx = ctx;

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

ZEND_BEGIN_ARG_INFO(arginfo_v8generator_current, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8generator_key, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8generator_next, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8generator_rewind, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_v8generator_valid, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry v8js_v8generator_methods[] = { /* {{{ */
	PHP_ME(V8Generator,	__construct,			NULL,							ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Generator,	__sleep,				NULL,							ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Generator,	__wakeup,				NULL,							ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)

	PHP_ME(V8Generator,	current,				arginfo_v8generator_current,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Generator,	key,					arginfo_v8generator_key,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Generator,	next,					arginfo_v8generator_next,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Generator,	rewind,					arginfo_v8generator_rewind,		ZEND_ACC_PUBLIC)
	PHP_ME(V8Generator,	valid,					arginfo_v8generator_valid,		ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */


PHP_MINIT_FUNCTION(v8js_v8object_class) /* {{{ */
{
	zend_class_entry ce;

	/* V8Object Class */
	INIT_CLASS_ENTRY(ce, "V8Object", v8js_v8object_methods);
	php_ce_v8object = zend_register_internal_class(&ce);
	php_ce_v8object->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8object->create_object = v8js_v8object_new;

	/* V8Function Class */
	INIT_CLASS_ENTRY(ce, "V8Function", v8js_v8function_methods);
	php_ce_v8function = zend_register_internal_class(&ce);
	php_ce_v8function->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8function->create_object = v8js_v8object_new;

	/* V8Generator Class */
	INIT_CLASS_ENTRY(ce, "V8Generator", v8js_v8generator_methods);
	php_ce_v8generator = zend_register_internal_class(&ce);
	php_ce_v8generator->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8generator->create_object = v8js_v8generator_new;

	zend_class_implements(php_ce_v8generator, 1, zend_ce_iterator);


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
	v8js_v8object_handlers.offset = XtOffsetOf(struct v8js_v8object, std);
	v8js_v8object_handlers.free_obj = v8js_v8object_free_storage;

	/* V8Generator handlers */
	memcpy(&v8js_v8generator_handlers, &v8js_v8object_handlers, sizeof(zend_object_handlers));
	v8js_v8generator_handlers.get_method = v8js_v8generator_get_method;
	v8js_v8generator_handlers.offset = XtOffsetOf(struct v8js_v8generator, v8obj.std);
	v8js_v8generator_handlers.free_obj = v8js_v8generator_free_storage;

	return SUCCESS;
} /* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
