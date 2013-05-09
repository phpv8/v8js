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
#include "ext/date/php_date.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
}

#include "php_v8js_macros.h"
#include <v8.h>

/* Callback for PHP methods and functions */
static v8::Handle<v8::Value> php_v8js_php_callback(const v8::Arguments &args) /* {{{ */
{
	v8::Handle<v8::Value> return_value;
	zval *value = reinterpret_cast<zval *>(args.This()->GetAlignedPointerFromInternalField(0));
	v8::Isolate *isolate = reinterpret_cast<v8::Isolate *>(args.This()->GetAlignedPointerFromInternalField(1));
	zend_function *method_ptr;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval fname, *retval_ptr = NULL, **argv = NULL;
	TSRMLS_FETCH();
	zend_class_entry *ce = Z_OBJCE_P(value);
	zend_uint argc = args.Length(), min_num_args = 0, max_num_args = 0;
	char *error;
	int error_len, i, flags = V8JS_FLAG_NONE;

	/* Set method_ptr from v8::External or fetch the closure invoker */
	if (!args.Data().IsEmpty() && args.Data()->IsExternal()) {
		method_ptr = static_cast<zend_function *>(v8::External::Cast(*args.Data())->Value());
	} else {
		method_ptr = zend_get_closure_invoke_method(value TSRMLS_CC);
	}

	/* Set parameter limits */
	min_num_args = method_ptr->common.required_num_args;
	max_num_args = method_ptr->common.num_args;

	/* Function name to call */
	ZVAL_STRING(&fname, method_ptr->common.function_name, 0);

	/* zend_fcall_info */
	fci.size = sizeof(fci);
	fci.function_table = &ce->function_table;
	fci.function_name = &fname;
	fci.symbol_table = NULL;
	fci.object_ptr = value;
	fci.retval_ptr_ptr = &retval_ptr;
	fci.param_count = 0;

	/* Check for passed vs required number of arguments */
	if (argc < min_num_args)
	{
		error_len = spprintf(&error, 0,
			"%s::%s() expects %s %d parameter%s, %d given",
				ce->name,
				method_ptr->common.function_name,
				min_num_args == max_num_args ? "exactly" : argc < min_num_args ? "at least" : "at most",
				argc < min_num_args ? min_num_args : max_num_args,
				(argc < min_num_args ? min_num_args : max_num_args) == 1 ? "" : "s",
				argc);

		return_value = V8JS_THROW(TypeError, error, error_len);
		if (ce == zend_ce_closure) {
			efree(const_cast<char*>(method_ptr->internal_function.function_name));
			efree(method_ptr);
		}
		efree(error);
		return return_value;
	}

	/* Convert parameters passed from V8 */
	if (argc) {
		flags = V8JS_GLOBAL_GET_FLAGS();
		fci.params = (zval ***) safe_emalloc(argc, sizeof(zval **), 0);
		argv = (zval **) safe_emalloc(argc, sizeof(zval *), 0);
		for (i = 0; i < argc; i++) {
			MAKE_STD_ZVAL(argv[i]);
			if (v8js_to_zval(args[i], argv[i], flags, isolate TSRMLS_CC) == FAILURE) {
				fci.param_count++;
				error_len = spprintf(&error, 0, "converting parameter #%d passed to %s() failed", i + 1, method_ptr->common.function_name);
				return_value = V8JS_THROW(Error, error, error_len);
				efree(error);
				goto failure;
			}
			fci.params[fci.param_count++] = &argv[i];
 		}
	} else {
		fci.params = NULL;
	}
	fci.no_separation = 1;

	/* zend_fcall_info_cache */
	fcc.initialized = 1;
	fcc.function_handler = method_ptr;
	fcc.calling_scope = ce;
	fcc.called_scope = ce;
	fcc.object_ptr = value;

	/* Call the method */
	zend_call_function(&fci, &fcc TSRMLS_CC);

failure:
	/* Cleanup */
	if (argc) {
		for (i = 0; i < fci.param_count; i++) {
			zval_ptr_dtor(&argv[i]);
		}
		efree(argv);
		efree(fci.params);
	}

	if (retval_ptr != NULL) {
		return_value = zval_to_v8js(retval_ptr, isolate TSRMLS_CC);
		zval_ptr_dtor(&retval_ptr);
	} else {
		return_value = V8JS_NULL;
	}

	return return_value;
}
/* }}} */

static int _php_v8js_is_assoc_array(HashTable *myht TSRMLS_DC) /* {{{ */
{
	int i;
	char *key;
	ulong index, idx = 0;
	uint key_len;
	HashPosition pos;

	zend_hash_internal_pointer_reset_ex(myht, &pos);
	for (;; zend_hash_move_forward_ex(myht, &pos)) {
		i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
		if (i == HASH_KEY_NON_EXISTANT)
			break;
		if (i == HASH_KEY_IS_STRING || index != idx) {
			return 1;
		}
		idx++;
	}
	return 0;
}
/* }}} */

static v8::Handle<v8::Value> php_v8js_property_caller(const v8::Arguments &args) /* {{{ */
{
	v8::Local<v8::Object> self = args.Holder();
	v8::Local<v8::String> cname = args.Callee()->GetName()->ToString();
	v8::Local<v8::Value> value;
	v8::Local<v8::String> cb_func = v8::Local<v8::String>::Cast(args.Data());

	value = self->GetHiddenValue(cb_func);

	if (!value.IsEmpty() && value->IsFunction())
	{
		int argc = args.Length(), i = 0;
		v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
		v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(value);

		if (cb_func->Equals(V8JS_SYM(ZEND_INVOKE_FUNC_NAME))) {
			for (; i < argc; ++i) {
				argv[i] = args[i];
			}
			value = cb->Call(self, argc, argv);
		}
		else /* __call() */
		{
			v8::Local<v8::Array> argsarr = v8::Array::New(argc);
			for (; i < argc; ++i) {
				argsarr->Set(i, args[i]);
			}
			v8::Local<v8::Value> argsv[2] = { cname, argsarr };
			value = cb->Call(self, 2, argsv);
		}
	}

	if (args.IsConstructCall()) {
		if (!value.IsEmpty() && !value->IsNull()) {
			return value;
		}
		return self;
	}

	return value;
}
/* }}} */

static v8::Handle<v8::Value> php_v8js_property_getter(v8::Local<v8::String> property, const v8::AccessorInfo &info) /* {{{ */
{
	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Value> value;
	v8::Local<v8::Function> cb;

	/* Check first if JS object has the named property */
	value = self->GetRealNamedProperty(property);

	if (!value.IsEmpty()) {
		return value;
	}

	/* If __get() is set for PHP object, call it */
	value = self->GetHiddenValue(V8JS_SYM(ZEND_GET_FUNC_NAME));
	if (!value.IsEmpty() && value->IsFunction()) {
		cb = v8::Local<v8::Function>::Cast(value);
		v8::Local<v8::Value> argv[1] = {property};
		value = cb->Call(self, 1, argv);
	}

	/* If __get() does not exist or returns NULL, create new function with callback for __call() */
	if ((value.IsEmpty() || value->IsNull()) && info.Data()->IsTrue()) {
		v8::Local<v8::FunctionTemplate> cb_t = v8::FunctionTemplate::New(php_v8js_property_caller, V8JS_SYM(ZEND_CALL_FUNC_NAME));
		cb = cb_t->GetFunction();
		cb->SetName(property);
		return cb;
	}

	return value;
}
/* }}} */

static v8::Handle<v8::Integer> php_v8js_property_query(v8::Local<v8::String> property, const v8::AccessorInfo &info) /* {{{ */
{
	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Value> value;

	/* Return early if property is set in JS object */
	if (self->HasRealNamedProperty(property)) {
		return V8JS_INT(v8::ReadOnly);
	}

	value = self->GetHiddenValue(V8JS_SYM(ZEND_ISSET_FUNC_NAME));
	if (!value.IsEmpty() && value->IsFunction()) {
		v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(value);
		v8::Local<v8::Value> argv[1] = {property};
		value = cb->Call(self, 1, argv);
	}

	return (!value.IsEmpty() && value->IsTrue()) ? V8JS_INT(v8::ReadOnly) : v8::Local<v8::Integer>();
}
/* }}} */

/* These are not defined by Zend */
#define ZEND_WAKEUP_FUNC_NAME    "__wakeup"
#define ZEND_SLEEP_FUNC_NAME     "__sleep"
#define ZEND_SET_STATE_FUNC_NAME "__set_state"

#define IS_MAGIC_FUNC(mname) \
	((key_len == sizeof(mname)) && \
	!strncasecmp(key, mname, key_len - 1))

#define PHP_V8JS_CALLBACK(mptr) \
	v8::FunctionTemplate::New(php_v8js_php_callback, v8::External::New(mptr))->GetFunction()

static v8::Handle<v8::Value> php_v8js_hash_to_jsobj(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	v8::Local<v8::FunctionTemplate> new_tpl = v8::FunctionTemplate::New();
	v8::Local<v8::Object> newobj;
	int i;
	char *key = NULL;
	ulong index;
	uint key_len;
	HashTable *myht;
	HashPosition pos;
	zend_class_entry *ce = NULL;
	zend_function *method_ptr, *call_ptr = NULL, *get_ptr = NULL, *invoke_ptr = NULL, *isset_ptr = NULL;

	if (Z_TYPE_P(value) == IS_ARRAY) {
		myht = HASH_OF(value);
	} else {
		myht = Z_OBJPROP_P(value);
		ce = Z_OBJCE_P(value);
	}

	/* Prevent recursion */
	if (myht && myht->nApplyCount > 1) {
		return V8JS_NULL;
	}

	/* Object methods */
	if (ce) {
		new_tpl->SetClassName(V8JS_STRL(ce->name, ce->name_length));
		new_tpl->InstanceTemplate()->SetInternalFieldCount(2);

		if (ce == zend_ce_closure) {
			new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(php_v8js_php_callback);
			newobj = new_tpl->InstanceTemplate()->NewInstance();
		} else {
			zend_hash_internal_pointer_reset_ex(&ce->function_table, &pos);
			for (;; zend_hash_move_forward_ex(&ce->function_table, &pos)) {
				if (zend_hash_get_current_key_ex(&ce->function_table, &key, &key_len, &index, 0, &pos) != HASH_KEY_IS_STRING  ||
					zend_hash_get_current_data_ex(&ce->function_table, (void **) &method_ptr, &pos) == FAILURE
				) {
					break;
				}

				if ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC)     && /* Allow only public methods */
					(method_ptr->common.fn_flags & ZEND_ACC_CTOR) == 0  && /* ..and no __construct() */
					(method_ptr->common.fn_flags & ZEND_ACC_DTOR) == 0  && /* ..or __destruct() */
					(method_ptr->common.fn_flags & ZEND_ACC_CLONE) == 0 /* ..or __clone() functions */
				) {
					/* Override native toString() with __tostring() if it is set in passed object */
					if (IS_MAGIC_FUNC(ZEND_TOSTRING_FUNC_NAME)) {
						new_tpl->InstanceTemplate()->Set(V8JS_SYM("toString"), PHP_V8JS_CALLBACK(method_ptr));
					/* TODO: __set(), __unset() disabled as JS is not allowed to modify the passed PHP object yet.
					 *  __sleep(), __wakeup(), __set_state() are always ignored */
					} else if (
						IS_MAGIC_FUNC(ZEND_CALLSTATIC_FUNC_NAME)|| /* TODO */
						IS_MAGIC_FUNC(ZEND_SLEEP_FUNC_NAME)     ||
						IS_MAGIC_FUNC(ZEND_WAKEUP_FUNC_NAME)    ||
						IS_MAGIC_FUNC(ZEND_SET_STATE_FUNC_NAME) ||
						IS_MAGIC_FUNC(ZEND_SET_FUNC_NAME)       ||
						IS_MAGIC_FUNC(ZEND_UNSET_FUNC_NAME)
					) {
					/* Register all magic function as hidden with lowercase name */
					} else if (IS_MAGIC_FUNC(ZEND_GET_FUNC_NAME)) {
						get_ptr = method_ptr;
					} else if (IS_MAGIC_FUNC(ZEND_CALL_FUNC_NAME)) {
						call_ptr = method_ptr;
					} else if (IS_MAGIC_FUNC(ZEND_INVOKE_FUNC_NAME)) {
						invoke_ptr = method_ptr;
					} else if (IS_MAGIC_FUNC(ZEND_ISSET_FUNC_NAME)) {
						isset_ptr = method_ptr;
					} else {
						new_tpl->InstanceTemplate()->Set(V8JS_STR(method_ptr->common.function_name), PHP_V8JS_CALLBACK(method_ptr), v8::ReadOnly);
					}
				}
			}

			/* Only register getter, etc. when they're set in PHP side */
			if (call_ptr || get_ptr || isset_ptr)
			{
				/* Set __get() handler which acts also as __call() proxy */
				new_tpl->InstanceTemplate()->SetNamedPropertyHandler(
					php_v8js_property_getter,					/* getter */
					0,											/* setter */
					isset_ptr ? php_v8js_property_query : 0,	/* query */
					0,											/* deleter */
					0,											/* enumerator */
					V8JS_BOOL(call_ptr ? true : false)
				);
			}

			/* __invoke() handler */
			if (invoke_ptr) {
				new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(php_v8js_property_caller, V8JS_SYM(ZEND_INVOKE_FUNC_NAME));
			}

			newobj = new_tpl->InstanceTemplate()->NewInstance();

			if (call_ptr) {
				newobj->SetHiddenValue(V8JS_SYM(ZEND_CALL_FUNC_NAME), PHP_V8JS_CALLBACK(call_ptr));
			}
			if (get_ptr) {
				newobj->SetHiddenValue(V8JS_SYM(ZEND_GET_FUNC_NAME), PHP_V8JS_CALLBACK(get_ptr));
			}
			if (invoke_ptr) {
				newobj->SetHiddenValue(V8JS_SYM(ZEND_INVOKE_FUNC_NAME), PHP_V8JS_CALLBACK(invoke_ptr));
			}
			if (isset_ptr) {
				newobj->SetHiddenValue(V8JS_SYM(ZEND_ISSET_FUNC_NAME), PHP_V8JS_CALLBACK(isset_ptr));
			}
		}
			// Increase the reference count of this value because we're storing it internally for use later
			// See https://github.com/preillyme/v8js/issues/6
			Z_ADDREF_P(value);

		newobj->SetAlignedPointerInInternalField(0, (void *) value);
		newobj->SetAlignedPointerInInternalField(1, (void *) isolate);
	} else {
		new_tpl->SetClassName(V8JS_SYM("Array"));
		newobj = new_tpl->InstanceTemplate()->NewInstance();
	}

	/* Object properties */
	i = myht ? zend_hash_num_elements(myht) : 0;

	if (i > 0)
	{
		zval **data;
		HashTable *tmp_ht;

		zend_hash_internal_pointer_reset_ex(myht, &pos);
		for (;; zend_hash_move_forward_ex(myht, &pos)) {
			i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
			if (i == HASH_KEY_NON_EXISTANT)
				break;

			if (zend_hash_get_current_data_ex(myht, (void **) &data, &pos) == SUCCESS)
			{
				tmp_ht = HASH_OF(*data);

				if (tmp_ht) {
					tmp_ht->nApplyCount++;
				}

				if (i == HASH_KEY_IS_STRING)
				{
					if (key[0] == '\0' && Z_TYPE_P(value) == IS_OBJECT) {
						/* Skip protected and private members. */
						if (tmp_ht) {
							tmp_ht->nApplyCount--;
						}
						continue;
					}
					newobj->Set(V8JS_STRL(key, key_len - 1), zval_to_v8js(*data, isolate TSRMLS_CC), v8::ReadOnly);
				} else {
					newobj->Set(index, zval_to_v8js(*data, isolate TSRMLS_CC));
				}

				if (tmp_ht) {
					tmp_ht->nApplyCount--;
				}
			}
		}
	}
	return newobj;
}
/* }}} */

static v8::Handle<v8::Value> php_v8js_hash_to_jsarr(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	HashTable *myht = HASH_OF(value);
	int i = myht ? zend_hash_num_elements(myht) : 0;

	/* Return object if dealing with assoc array */
	if (i > 0 && _php_v8js_is_assoc_array(myht TSRMLS_CC)) {
		return php_v8js_hash_to_jsobj(value, isolate TSRMLS_CC);
	}

	v8::Local<v8::Array> newarr;

	/* Prevent recursion */
	if (myht && myht->nApplyCount > 1) {
		return V8JS_NULL;
	}

	newarr = v8::Array::New(i);

	if (i > 0)
	{
		zval **data;
		ulong index = 0;
		HashTable *tmp_ht;
		HashPosition pos;

		for (zend_hash_internal_pointer_reset_ex(myht, &pos);
			SUCCESS == zend_hash_get_current_data_ex(myht, (void **) &data, &pos);
			zend_hash_move_forward_ex(myht, &pos)
		) {
			tmp_ht = HASH_OF(*data);

			if (tmp_ht) {
				tmp_ht->nApplyCount++;
			}

			newarr->Set(index++, zval_to_v8js(*data, isolate TSRMLS_CC));

			if (tmp_ht) {
				tmp_ht->nApplyCount--;
			}
		}
	}
	return newarr;
}
/* }}} */

v8::Handle<v8::Value> zval_to_v8js(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	v8::Handle<v8::Value> jsValue;

	switch (Z_TYPE_P(value))
	{
		case IS_ARRAY:
			jsValue = php_v8js_hash_to_jsarr(value, isolate TSRMLS_CC);
			break;

		case IS_OBJECT:
			jsValue = php_v8js_hash_to_jsobj(value, isolate TSRMLS_CC);
			break;

		case IS_STRING:
			jsValue = V8JS_STRL(Z_STRVAL_P(value), Z_STRLEN_P(value));
			break;

		case IS_LONG:
			jsValue = V8JS_INT(Z_LVAL_P(value));
			break;

		case IS_DOUBLE:
			jsValue = V8JS_FLOAT(Z_DVAL_P(value));
			break;

		case IS_BOOL:
			jsValue = V8JS_BOOL(Z_BVAL_P(value));
			break;

		default:
		case IS_NULL:
			jsValue = V8JS_NULL;
			break;
	}
	return jsValue;
}
/* }}} */

int v8js_to_zval(v8::Handle<v8::Value> jsValue, zval *return_value, int flags, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	if (jsValue->IsString())
	{
		v8::String::Utf8Value str(jsValue);
		const char *cstr = ToCString(str);
		RETVAL_STRING(cstr, 1);
	}
	else if (jsValue->IsBoolean())
	{
		RETVAL_BOOL(jsValue->Uint32Value());
	}
	else if (jsValue->IsInt32() || jsValue->IsUint32())
	{
		RETVAL_LONG((long) jsValue->IntegerValue());
	}
	else if (jsValue->IsNumber())
	{
		RETVAL_DOUBLE(jsValue->NumberValue());
	}
	else if (jsValue->IsDate())	/* Return as a PHP DateTime object */
	{
		v8::String::Utf8Value str(jsValue);
		const char *cstr = ToCString(str);
		zend_class_entry *ce = php_date_get_date_ce();
#if PHP_VERSION_ID < 50304
		zval *param;

		MAKE_STD_ZVAL(param);
		ZVAL_STRING(param, cstr, 1);

		object_init_ex(return_value, ce TSRMLS_CC);
		zend_call_method_with_1_params(&return_value, ce, &ce->constructor, "__construct", NULL, param);
		zval_ptr_dtor(&param);

		if (EG(exception)) {
			return FAILURE;
		}
#else
		php_date_instantiate(ce, return_value TSRMLS_CC);
		if (!php_date_initialize((php_date_obj *) zend_object_store_get_object(return_value TSRMLS_CC), (char *) cstr, strlen(cstr), NULL, NULL, 0 TSRMLS_CC)) {
			return FAILURE;
		}
#endif
	}
	else if (jsValue->IsObject())
	{
		if ((flags & V8JS_FLAG_FORCE_ARRAY) || jsValue->IsArray()) {
			array_init(return_value);
			return php_v8js_v8_get_properties_hash(jsValue, Z_ARRVAL_P(return_value), flags, isolate TSRMLS_CC);
		} else {
			php_v8js_create_v8(return_value, jsValue, flags, isolate TSRMLS_CC);
			return SUCCESS;
		}
	}
	else /* types External, RegExp, Undefined and Null are considered NULL */
	{
		RETVAL_NULL();
	}

 	return SUCCESS;
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
