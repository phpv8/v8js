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
}

#include "php_v8js_macros.h"
#include "v8js_array_access.h"
#include "v8js_object_export.h"
#include "v8js_v8object_class.h"

static void v8js_weak_object_callback(const v8::WeakCallbackData<v8::Object, zval> &data);

/* Callback for PHP methods and functions */
static void v8js_call_php_func(zval *value, zend_class_entry *ce, zend_function *method_ptr, v8::Isolate *isolate, const v8::FunctionCallbackInfo<v8::Value>& info TSRMLS_DC) /* {{{ */
{
	v8::Handle<v8::Value> return_value;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval fname, *retval_ptr = NULL, **argv = NULL;
	zend_uint argc = info.Length(), min_num_args = 0, max_num_args = 0;
	char *error;
	int error_len, i, flags = V8JS_FLAG_NONE;

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	/* Set parameter limits */
	min_num_args = method_ptr->common.required_num_args;
	max_num_args = method_ptr->common.num_args;

	/* Function name to call */
	INIT_ZVAL(fname);
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

		return_value = V8JS_THROW(isolate, TypeError, error, error_len);
		if (ce == zend_ce_closure) {
			efree(const_cast<char*>(method_ptr->internal_function.function_name));
			efree(method_ptr);
		}
		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	/* Convert parameters passed from V8 */
	if (argc) {
		flags = V8JS_GLOBAL_GET_FLAGS(isolate);
		fci.params = (zval ***) safe_emalloc(argc, sizeof(zval **), 0);
		argv = (zval **) safe_emalloc(argc, sizeof(zval *), 0);
		for (i = 0; i < argc; i++) {
			v8::Local<v8::Value> php_object;
			if (info[i]->IsObject()) {
				php_object = v8::Local<v8::Object>::Cast(info[i])->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY));
			}
			if (!php_object.IsEmpty()) {
				/* This is a PHP object, passed to JS and back. */
				argv[i] = reinterpret_cast<zval *>(v8::External::Cast(*php_object)->Value());
				Z_ADDREF_P(argv[i]);
			} else {
				MAKE_STD_ZVAL(argv[i]);
				if (v8js_to_zval(info[i], argv[i], flags, isolate TSRMLS_CC) == FAILURE) {
					fci.param_count++;
					error_len = spprintf(&error, 0, "converting parameter #%d passed to %s() failed", i + 1, method_ptr->common.function_name);
					return_value = V8JS_THROW(isolate, Error, error, error_len);
					efree(error);
					goto failure;
				}
			}

			fci.params[fci.param_count++] = &argv[i];
 		}
	} else {
		fci.params = NULL;
	}

	fci.no_separation = 1;
	info.GetReturnValue().Set(V8JS_NULL);

	zend_try {
		{
			isolate->Exit();
			v8::Unlocker unlocker(isolate);

			/* zend_fcall_info_cache */
			fcc.initialized = 1;
			fcc.function_handler = method_ptr;
			fcc.calling_scope = ce;
			fcc.called_scope = ce;
			fcc.object_ptr = value;

			zend_call_function(&fci, &fcc TSRMLS_CC);
		}

		isolate->Enter();
	}
	zend_catch {
		v8::V8::TerminateExecution(isolate);
		V8JSG(fatal_error_abort) = 1;
	}
	zend_end_try();

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

	info.GetReturnValue().Set(return_value);
}
/* }}} */

/* Callback for PHP methods and functions */
static void v8js_php_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();

	V8JS_TSRMLS_FETCH();
	zval *value = reinterpret_cast<zval *>(v8::External::Cast(*self->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY)))->Value());
	zend_function *method_ptr;
	zend_class_entry *ce = Z_OBJCE_P(value);

	/* Set method_ptr from v8::External or fetch the closure invoker */
	if (!info.Data().IsEmpty() && info.Data()->IsExternal()) {
		method_ptr = static_cast<zend_function *>(v8::External::Cast(*info.Data())->Value());
	} else {
		method_ptr = zend_get_closure_invoke_method(value TSRMLS_CC);
	}

	return v8js_call_php_func(value, ce, method_ptr, isolate, info TSRMLS_CC);
}

/* Callback for PHP constructor calls */
static void v8js_construct_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	info.GetReturnValue().Set(V8JS_UNDEFINED);

	// @todo assert constructor call
	v8::Handle<v8::Object> newobj = info.This();
	v8::Local<v8::External> php_object;
	zval *value;

	if (!info.IsConstructCall()) {
		return;
	}

	v8::Local<v8::Array> cons_data = v8::Local<v8::Array>::Cast(info.Data());
	v8::Local<v8::External> ext_tmpl = v8::Local<v8::External>::Cast(cons_data->Get(0));
	v8::Local<v8::External> ext_ce =  v8::Local<v8::External>::Cast(cons_data->Get(1));

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	if (info[0]->IsExternal()) {
		// Object created by v8js in v8js_hash_to_jsobj, PHP object passed as v8::External.
		php_object = v8::Local<v8::External>::Cast(info[0]);
		value = reinterpret_cast<zval *>(php_object->Value());

		if(ctx->weak_objects.count(value)) {
			// We already exported this object, hence no need to add another
			// ref, v8 won't give us a second weak-object callback anyways.
			newobj->SetAlignedPointerInInternalField(0, ext_tmpl->Value());
			newobj->SetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY), php_object);
			return;
		}

		// Increase the reference count of this value because we're storing it internally for use later
		// See https://github.com/preillyme/v8js/issues/6
		Z_ADDREF_P(value);
	} else {
		// Object created from JavaScript context.  Need to create PHP object first.
		V8JS_TSRMLS_FETCH();
		zend_class_entry *ce = static_cast<zend_class_entry *>(ext_ce->Value());
		zend_function *ctor_ptr = ce->constructor;

		// Check access on __construct function, if any
		if (ctor_ptr != NULL && (ctor_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0) {
			info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Call to protected __construct() not allowed")));
			return;
		}

		MAKE_STD_ZVAL(value);
		object_init_ex(value, ce);

		// Call __construct function
		if (ctor_ptr != NULL) {
			v8js_call_php_func(value, ce, ctor_ptr, isolate, info TSRMLS_CC);
		}
		php_object = v8::External::New(isolate, value);
	}

	newobj->SetAlignedPointerInInternalField(0, ext_tmpl->Value());
	newobj->SetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY), php_object);

	// Since we got to decrease the reference count again, in case v8 garbage collector
	// decides to dispose the JS object, we add a weak persistent handle and register
	// a callback function that removes the reference.
	ctx->weak_objects[value].Reset(isolate, newobj);
	ctx->weak_objects[value].SetWeak(value, v8js_weak_object_callback);

	// Just tell v8 that we're allocating some external memory
	// (for the moment we just always tell 1k instead of trying to find out actual values)
	isolate->AdjustAmountOfExternalAllocatedMemory(1024);
}
/* }}} */


static void v8js_weak_object_callback(const v8::WeakCallbackData<v8::Object, zval> &data) {
	v8::Isolate *isolate = data.GetIsolate();

	zval *value = data.GetParameter();
	zval_ptr_dtor(&value);

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	ctx->weak_objects.at(value).Reset();
	ctx->weak_objects.erase(value);

	isolate->AdjustAmountOfExternalAllocatedMemory(-1024);
}

static void v8js_weak_closure_callback(const v8::WeakCallbackData<v8::Object, v8js_tmpl_t> &data) {
	v8::Isolate *isolate = data.GetIsolate();

	v8js_tmpl_t *persist_tpl_ = data.GetParameter();
	persist_tpl_->Reset();
	delete persist_tpl_;

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	ctx->weak_closures.at(persist_tpl_).Reset();
	ctx->weak_closures.erase(persist_tpl_);
};

/* These are not defined by Zend */
#define ZEND_WAKEUP_FUNC_NAME    "__wakeup"
#define ZEND_SLEEP_FUNC_NAME     "__sleep"
#define ZEND_SET_STATE_FUNC_NAME "__set_state"

#define IS_MAGIC_FUNC(mname) \
	((key_len == sizeof(mname)) && \
	!strncasecmp(key, mname, key_len - 1))

#define PHP_V8JS_CALLBACK(isolate, mptr, tmpl)										\
	v8::FunctionTemplate::New((isolate), v8js_php_callback, v8::External::New((isolate), mptr), v8::Signature::New((isolate), tmpl))->GetFunction()


static void v8js_named_property_enumerator(const v8::PropertyCallbackInfo<v8::Array> &info) /* {{{ */
{
	// note: 'special' properties like 'constructor' are not enumerated.
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Array> result = v8::Array::New(isolate, 0);
	uint32_t result_len = 0;

	V8JS_TSRMLS_FETCH();
	zend_class_entry *ce;
	zend_function *method_ptr;
	HashTable *proptable;
	HashPosition pos;
	char *key = NULL;
	uint key_len;
	ulong index;

	zval *object = reinterpret_cast<zval *>(v8::External::Cast(*self->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY)))->Value());
	ce = Z_OBJCE_P(object);

	/* enumerate all methods */
	zend_hash_internal_pointer_reset_ex(&ce->function_table, &pos);
	for (;; zend_hash_move_forward_ex(&ce->function_table, &pos)) {
		if (zend_hash_get_current_key_ex(&ce->function_table, &key, &key_len, &index, 0, &pos) != HASH_KEY_IS_STRING  ||
			zend_hash_get_current_data_ex(&ce->function_table, (void **) &method_ptr, &pos) == FAILURE
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
		// hide (do not enumerate) other PHP magic functions
		if (IS_MAGIC_FUNC(ZEND_CALLSTATIC_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SLEEP_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_WAKEUP_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SET_STATE_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_GET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_SET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_UNSET_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_CALL_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_INVOKE_FUNC_NAME) ||
			IS_MAGIC_FUNC(ZEND_ISSET_FUNC_NAME)) {
			continue;
		}
		v8::Local<v8::String> method_name = V8JS_STR(method_ptr->common.function_name);
		// rename PHP special method names to JS equivalents.
		if (IS_MAGIC_FUNC(ZEND_TOSTRING_FUNC_NAME)) {
			method_name = V8JS_SYM("toString");
		}
		result->Set(result_len++, method_name);
	}
	/* enumerate all properties */
	/* Z_OBJPROP uses the get_properties handler */
	proptable = Z_OBJPROP_P(object);
	zend_hash_internal_pointer_reset_ex(proptable, &pos);
	for (;; zend_hash_move_forward_ex(proptable, &pos)) {
		int i = zend_hash_get_current_key_ex(proptable, &key, &key_len, &index, 0, &pos);
		if (i == HASH_KEY_NON_EXISTANT)
			break;

		// for consistency with the 'in' operator, skip properties whose
		// value IS_NULL (like isset does)
		zval **data;
		if (zend_hash_get_current_data_ex(proptable, (void **) &data, &pos) == SUCCESS &&
			ZVAL_IS_NULL(*data))
			continue;

		if (i == HASH_KEY_IS_STRING) {
			/* skip protected and private members */
			if (key[0] == '\0') {
				continue;
			}
			// prefix enumerated property names with '$' so they can be
			// dereferenced unambiguously (ie, don't conflict with method
			// names)
			char *prefixed = static_cast<char *>(emalloc(key_len + 1));
			prefixed[0] = '$';
			strncpy(prefixed + 1, key, key_len);
			result->Set(result_len++, V8JS_STRL(prefixed, key_len));
			efree(prefixed);
		} else {
			// even numeric indices are enumerated as strings in JavaScript
			result->Set(result_len++, V8JS_FLOAT((double) index)->ToString());
		}
	}

	/* done */
	info.GetReturnValue().Set(result);
}
/* }}} */

static void v8js_invoke_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info.Data());
	int argc = info.Length(), i;
	v8::Local<v8::Value> *argv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));
	v8::Local<v8::Value> result;

	V8JS_TSRMLS_FETCH();

	for (i=0; i<argc; i++) {
		new(&argv[i]) v8::Local<v8::Value>;
		argv[i] = info[i];
	}

	if (info.IsConstructCall()) {
		v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

		v8::String::Utf8Value str(self->GetConstructorName()->ToString());
		const char *constructor_name = ToCString(str);

		zend_class_entry **pce;
		zend_lookup_class(constructor_name, str.length(), &pce TSRMLS_CC);
		v8::Local<v8::FunctionTemplate> new_tpl;
		new_tpl = v8::Local<v8::FunctionTemplate>::New
			(isolate, ctx->template_cache.at((*pce)->name));

		result = new_tpl->GetFunction()->NewInstance(argc, argv);
	} else {
		result = cb->Call(self, argc, argv);
	}
	info.GetReturnValue().Set(result);
}
/* }}} */

// this is a magic '__call' implementation for PHP classes which don't actually
// have a '__call' magic function.  This way we can always force a method
// call (as opposed to a property get) from JavaScript using __call.
static void v8js_fake_call_impl(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.Holder();
	v8::Handle<v8::Value> return_value;

	char *error;
	int error_len;

	V8JS_TSRMLS_FETCH();
	zend_class_entry *ce;
	zval *object = reinterpret_cast<zval *>(v8::External::Cast(*self->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY)))->Value());
	ce = Z_OBJCE_P(object);

	// first arg is method name, second arg is array of args.
	if (info.Length() < 2) {
		error_len = spprintf(&error, 0,
			"%s::__call expects 2 parameters, %d given",
			ce->name, (int) info.Length());
		return_value = V8JS_THROW(isolate, TypeError, error, error_len);
		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}
	if (!info[1]->IsArray()) {
		error_len = spprintf(&error, 0,
			"%s::__call expects 2nd parameter to be an array",
			ce->name);
		return_value = V8JS_THROW(isolate, TypeError, error, error_len);
		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}
	v8::String::Utf8Value str(info[0]->ToString());
	const char *method_name = ToCString(str);
	uint method_name_len = strlen(method_name);
	v8::Local<v8::Array> args = v8::Local<v8::Array>::Cast(info[1]);
	if (args->Length() > 1000000) {
		// prevent overflow, since args->Length() is a uint32_t and args
		// in the Function->Call method below is a (signed) int.
		error_len = spprintf(&error, 0,
			"%s::__call expects fewer than a million arguments",
			ce->name);
		return_value = V8JS_THROW(isolate, TypeError, error, error_len);
		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}
	// okay, look up the method name and manually invoke it.
	const zend_object_handlers *h = Z_OBJ_HT_P(object);
	zend_function *method_ptr =
		h->get_method(&object, (char*)method_name, method_name_len
			ZEND_HASH_KEY_NULL TSRMLS_CC);
	if (method_ptr == NULL ||
		(method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0 ||
		(method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR|ZEND_ACC_CLONE)) != 0) {
		error_len = spprintf(&error, 0,
			"%s::__call to %s method %s", ce->name,
			(method_ptr == NULL) ? "undefined" : "non-public", method_name);
		return_value = V8JS_THROW(isolate, TypeError, error, error_len);
		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	v8::Local<v8::FunctionTemplate> tmpl =
		v8::Local<v8::FunctionTemplate>::New
			(isolate, *reinterpret_cast<v8js_tmpl_t *>(self->GetAlignedPointerFromInternalField(0)));
	// use v8js_php_callback to actually execute the method
	v8::Local<v8::Function> cb = PHP_V8JS_CALLBACK(isolate, method_ptr, tmpl);
	uint32_t i, argc = args->Length();
	v8::Local<v8::Value> *argv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));
	for (i=0; i<argc; i++) {
		new(&argv[i]) v8::Local<v8::Value>;
		argv[i] = args->Get(i);
	}
	return_value = cb->Call(info.This(), (int) argc, argv);
	info.GetReturnValue().Set(return_value);
}
/* }}} */

/* This method handles named property and method get/set/query/delete. */
template<typename T>
inline v8::Local<v8::Value> v8js_named_property_callback(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<T> &info, property_op_t callback_type, v8::Local<v8::Value> set_value) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::String::Utf8Value cstr(property);
	const char *name = ToCString(cstr);
	uint name_len = strlen(name);
	char *lower = estrndup(name, name_len);
	const char *method_name;
	uint method_name_len;

	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Value> ret_value;
	v8::Local<v8::Function> cb;

	V8JS_TSRMLS_FETCH();
	zend_class_entry *scope, *ce;
	zend_function *method_ptr = NULL;
	zval *php_value;

	zval *object = reinterpret_cast<zval *>(v8::External::Cast(*self->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY)))->Value());
	v8::Local<v8::FunctionTemplate> tmpl =
		v8::Local<v8::FunctionTemplate>::New
		(isolate, *reinterpret_cast<v8js_tmpl_t *>(self->GetAlignedPointerFromInternalField(0)));
	ce = scope = Z_OBJCE_P(object);

	/* First, check the (case-insensitive) method table */
	php_strtolower(lower, name_len);
	method_name = lower;
	method_name_len = name_len;
	// toString() -> __tostring()
	if (name_len == 8 && strcmp(name, "toString") == 0) {
		method_name = ZEND_TOSTRING_FUNC_NAME;
		method_name_len = sizeof(ZEND_TOSTRING_FUNC_NAME) - 1;
	}
	bool is_constructor = (name_len == 11 && strcmp(name, "constructor") == 0);
	bool is_magic_call = (method_name_len == 6 && strcmp(method_name, "__call") == 0);
	if (is_constructor ||
		(name[0] != '$' /* leading '$' means property, not method */ &&
		 zend_hash_find(&ce->function_table, method_name, method_name_len + 1, (void**)&method_ptr) == SUCCESS &&
		 ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) != 0) && /* Allow only public methods */
		 ((method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR|ZEND_ACC_CLONE)) == 0) /* no __construct, __destruct(), or __clone() functions */
		 ) || (method_ptr=NULL, is_magic_call)
	) {
		if (callback_type == V8JS_PROP_GETTER) {
			if (is_constructor) {
				// Don't set a return value here, i.e. indicate that we don't
				// have a special value.  V8 "knows" the constructor anyways
				// (from the template) and will use that.
			} else {
				if (is_magic_call && method_ptr==NULL) {
					// Fake __call implementation
					// (only use this if method_ptr==NULL, which means
					//  there is no actual PHP __call() implementation)
					v8::Local<v8::Function> cb =
						v8::FunctionTemplate::New(isolate,
							v8js_fake_call_impl, V8JS_NULL,
							v8::Signature::New(isolate, tmpl))->GetFunction();
					cb->SetName(property);
					ret_value = cb;
				} else {
					ret_value = PHP_V8JS_CALLBACK(isolate, method_ptr, tmpl);
				}
			}
		} else if (callback_type == V8JS_PROP_QUERY) {
			// methods are not enumerable
			ret_value = V8JS_UINT(v8::ReadOnly|v8::DontEnum|v8::DontDelete);
		} else if (callback_type == V8JS_PROP_SETTER) {
			ret_value = set_value; // lie.  this field is read-only.
		} else if (callback_type == V8JS_PROP_DELETER) {
			ret_value = V8JS_BOOL(false);
		} else {
			/* shouldn't reach here! but bail safely */
			ret_value = v8::Handle<v8::Value>();
		}
	} else {
		if (name[0]=='$') {
			// this is a property (not a method)
			name++; name_len--;
		}
		if (callback_type == V8JS_PROP_GETTER) {
			/* Nope, not a method -- must be a (case-sensitive) property */
			zval zname;
			INIT_ZVAL(zname);
			ZVAL_STRINGL(&zname, name, name_len, 0);
			zend_property_info *property_info = zend_get_property_info(ce, &zname, 1 TSRMLS_CC);

			if(property_info && property_info->flags & ZEND_ACC_PUBLIC) {
				php_value = zend_read_property(NULL, object, V8JS_CONST name, name_len, true TSRMLS_CC);
				// special case uninitialized_zval_ptr and return an empty value
				// (indicating that we don't intercept this property) if the
				// property doesn't exist.
				if (php_value == EG(uninitialized_zval_ptr)) {
					ret_value = v8::Handle<v8::Value>();
				} else {
					// wrap it
					ret_value = zval_to_v8js(php_value, isolate TSRMLS_CC);
					/* We don't own the reference to php_value... unless the
					 * returned refcount was 0, in which case the below code
					 * will free it. */
					zval_add_ref(&php_value);
					zval_ptr_dtor(&php_value);
				}
			}
			else if (zend_hash_find(&ce->function_table, "__get", 6, (void**)&method_ptr) == SUCCESS
					 /* Allow only public methods */
					 && ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) != 0)) {
				/* Okay, let's call __get. */
				zend_fcall_info fci;

				zval fmember;
				INIT_ZVAL(fmember);
				ZVAL_STRING(&fmember, "__get", 0);

				fci.size = sizeof(fci);
				fci.function_table = &ce->function_table;
				fci.function_name = &fmember;
				fci.symbol_table = NULL;
				fci.retval_ptr_ptr = &php_value;

				zval *zname_ptr = &zname;
				zval **zname_ptr_ptr = &zname_ptr;
				fci.param_count = 1;
				fci.params = &zname_ptr_ptr;

				fci.object_ptr = object;
				fci.no_separation = 0;

				zend_call_function(&fci, NULL TSRMLS_CC);

				ret_value = zval_to_v8js(php_value, isolate TSRMLS_CC);
				zval_ptr_dtor(&php_value);
			}
		} else if (callback_type == V8JS_PROP_SETTER) {
			MAKE_STD_ZVAL(php_value);
			if (v8js_to_zval(set_value, php_value, 0, isolate TSRMLS_CC) != SUCCESS) {
				ret_value = v8::Handle<v8::Value>();
			}
			else {
				zval zname;
				INIT_ZVAL(zname);
				ZVAL_STRINGL(&zname, name, name_len, 0);
				zend_property_info *property_info = zend_get_property_info(ce, &zname, 1 TSRMLS_CC);

				if(property_info && property_info->flags & ZEND_ACC_PUBLIC) {
					zend_update_property(scope, object, V8JS_CONST name, name_len, php_value TSRMLS_CC);
					ret_value = set_value;
				}
				else if (zend_hash_find(&ce->function_table, "__set", 6, (void**)&method_ptr) == SUCCESS
						 /* Allow only public methods */
						 && ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) != 0)) {
					/* Okay, let's call __set. */
					zend_fcall_info fci;

					zval fmember;
					INIT_ZVAL(fmember);
					ZVAL_STRING(&fmember, "__set", 0);

					zval *php_ret_value;

					fci.size = sizeof(fci);
					fci.function_table = &ce->function_table;
					fci.function_name = &fmember;
					fci.symbol_table = NULL;
					fci.retval_ptr_ptr = &php_ret_value;

					zval *zname_ptr = &zname;
					zval **params[2];
					fci.param_count = 2;
					fci.params = params;
					fci.params[0] = &zname_ptr;
					fci.params[1] = &php_value;

					fci.object_ptr = object;
					fci.no_separation = 1;

					zend_call_function(&fci, NULL TSRMLS_CC);
					ret_value = zval_to_v8js(php_ret_value, isolate TSRMLS_CC);
					zval_ptr_dtor(&php_ret_value);
				}
			}

			// if PHP wanted to hold on to this value, update_property would
			// have bumped the refcount
			zval_ptr_dtor(&php_value);
		} else if (callback_type == V8JS_PROP_QUERY ||
				   callback_type == V8JS_PROP_DELETER) {
			const zend_object_handlers *h = Z_OBJ_HT_P(object);
			zval *prop;
			MAKE_STD_ZVAL(prop);
			ZVAL_STRINGL(prop, name, name_len, 1);

			if (callback_type == V8JS_PROP_QUERY) {
				if (h->has_property(object, prop, 0 ZEND_HASH_KEY_NULL TSRMLS_CC)) {
					ret_value = V8JS_UINT(v8::None);
				} else {
					ret_value = v8::Handle<v8::Value>(); // empty handle
				}
			} else {
				zend_property_info *property_info = zend_get_property_info(ce, prop, 1 TSRMLS_CC);

				if(property_info && property_info->flags & ZEND_ACC_PUBLIC) {
					h->unset_property(object, prop ZEND_HASH_KEY_NULL TSRMLS_CC);
					ret_value = V8JS_BOOL(true);
				}
				else {
					ret_value = v8::Handle<v8::Value>(); // empty handle
				}
			}
			zval_ptr_dtor(&prop);
		} else {
			/* shouldn't reach here! but bail safely */
			ret_value = v8::Handle<v8::Value>();
		}
	}

	efree(lower);
	return ret_value;
}
/* }}} */

static void v8js_named_property_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> &info) /* {{{ */
{
	info.GetReturnValue().Set(v8js_named_property_callback(property, info, V8JS_PROP_GETTER));
}
/* }}} */

static void v8js_named_property_setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info) /* {{{ */
{
	info.GetReturnValue().Set(v8js_named_property_callback(property, info, V8JS_PROP_SETTER, value));
}
/* }}} */

static void v8js_named_property_query(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Integer> &info) /* {{{ */
{
	v8::Local<v8::Value> r = v8js_named_property_callback(property, info, V8JS_PROP_QUERY);
	if (!r.IsEmpty()) {
		info.GetReturnValue().Set(r->ToInteger());
	}
}
/* }}} */

static void v8js_named_property_deleter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Boolean> &info) /* {{{ */
{
	v8::Local<v8::Value> r = v8js_named_property_callback(property, info, V8JS_PROP_DELETER);
	if (!r.IsEmpty()) {
		info.GetReturnValue().Set(r->ToBoolean());
	}
}
/* }}} */



static v8::Handle<v8::Object> v8js_wrap_object(v8::Isolate *isolate, zend_class_entry *ce, zval *value TSRMLS_DC) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::Local<v8::FunctionTemplate> new_tpl;
	v8js_tmpl_t *persist_tpl_;

	try {
		new_tpl = v8::Local<v8::FunctionTemplate>::New
			(isolate, ctx->template_cache.at(ce->name));
	}
	catch (const std::out_of_range &) {
		/* No cached v8::FunctionTemplate available as of yet, create one. */
		new_tpl = v8::FunctionTemplate::New(isolate, 0);

		new_tpl->SetClassName(V8JS_STRL(ce->name, ce->name_length));
		new_tpl->InstanceTemplate()->SetInternalFieldCount(1);

		if (ce == zend_ce_closure) {
			/* Got a closure, mustn't cache ... */
			persist_tpl_ = new v8js_tmpl_t(isolate, new_tpl);
			/* We'll free persist_tpl_ via v8js_weak_closure_callback, below */
			new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(v8js_php_callback);
		} else {
			/* Add new v8::FunctionTemplate to tpl_map, as long as it is not a closure. */
			persist_tpl_ = &ctx->template_cache[ce->name];
			persist_tpl_->Reset(isolate, new_tpl);
			/* We'll free persist_tpl_ when template_cache is destroyed */

			v8::Local<v8::ObjectTemplate> inst_tpl = new_tpl->InstanceTemplate();
			v8::NamedPropertyGetterCallback getter = v8js_named_property_getter;
			v8::NamedPropertyEnumeratorCallback enumerator = v8js_named_property_enumerator;

			/* Check for ArrayAccess object */
			if (V8JSG(use_array_access) && ce) {
				bool has_array_access = false;
				bool has_countable = false;

				for (int i = 0; i < ce->num_interfaces; i ++) {
					if (strcmp (ce->interfaces[i]->name, "ArrayAccess") == 0) {
						has_array_access = true;
					}
					else if (strcmp (ce->interfaces[i]->name, "Countable") == 0) {
						has_countable = true;
					}
				}

				if(has_array_access && has_countable) {
					inst_tpl->SetIndexedPropertyHandler(v8js_array_access_getter,
														v8js_array_access_setter,
														v8js_array_access_query,
														v8js_array_access_deleter,
														v8js_array_access_enumerator);

					/* Switch to special ArrayAccess getter, which falls back to
					 * v8js_named_property_getter, but possibly bridges the
					 * call to Array.prototype functions. */
					getter = v8js_array_access_named_getter;

					/* Overwrite enumerator, since for(... in ...) loop should
					 * not see the methods but iterate over the elements. */
					enumerator = 0;
				}
			}


			// Finish setup of new_tpl
			inst_tpl->SetNamedPropertyHandler
				(getter, /* getter */
				 v8js_named_property_setter, /* setter */
				 v8js_named_property_query, /* query */
				 v8js_named_property_deleter, /* deleter */
				 enumerator, /* enumerator */
				 V8JS_NULL /* data */
				 );
			// add __invoke() handler
			zend_function *invoke_method_ptr;
			if (zend_hash_find(&ce->function_table, ZEND_INVOKE_FUNC_NAME,
							   sizeof(ZEND_INVOKE_FUNC_NAME),
							   (void**)&invoke_method_ptr) == SUCCESS &&
				invoke_method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) {
				new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(v8js_invoke_callback, PHP_V8JS_CALLBACK(isolate, invoke_method_ptr, new_tpl));
			}
		}
		v8::Local<v8::Array> call_handler_data = v8::Array::New(isolate, 2);
		call_handler_data->Set(0, v8::External::New(isolate, persist_tpl_));
		call_handler_data->Set(1, v8::External::New(isolate, ce));
		new_tpl->SetCallHandler(v8js_construct_callback, call_handler_data);
	}

	// Create v8 wrapper object
	v8::Handle<v8::Value> external = v8::External::New(isolate, value);
	v8::Handle<v8::Object> newobj = new_tpl->GetFunction()->NewInstance(1, &external);

	if (ce == zend_ce_closure) {
		// free uncached function template when object is freed
		ctx->weak_closures[persist_tpl_].Reset(isolate, newobj);
		ctx->weak_closures[persist_tpl_].SetWeak(persist_tpl_, v8js_weak_closure_callback);
	}

	return newobj;
}
/* }}} */


static v8::Handle<v8::Object> v8js_wrap_array_to_object(v8::Isolate *isolate, zval *value TSRMLS_DC) /* {{{ */
{
	int i;
	HashPosition pos;
	char *key = NULL;
	uint key_len;
	ulong index;

	// @todo re-use template likewise
	v8::Local<v8::FunctionTemplate> new_tpl = v8::FunctionTemplate::New(isolate, 0);

	/* Call it Array, but it is not a native array, especially it doesn't have
	 * have the typical Array.prototype functions. */
	new_tpl->SetClassName(V8JS_SYM("Array"));

	v8::Handle<v8::Object> newobj = new_tpl->InstanceTemplate()->NewInstance();

	HashTable *myht = HASH_OF(value);
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
					newobj->Set(V8JS_STRL(key, key_len - 1), zval_to_v8js(*data, isolate TSRMLS_CC));
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


v8::Handle<v8::Value> v8js_hash_to_jsobj(zval *value, v8::Isolate *isolate TSRMLS_DC) /* {{{ */
{
	HashTable *myht;
	zend_class_entry *ce = NULL;

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

	/* Special case, passing back object originating from JS to JS */
	if (ce == php_ce_v8function) {
		v8js_v8object *c = (v8js_v8object *) zend_object_store_get_object(value TSRMLS_CC);

		if(isolate != c->ctx->isolate) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "V8Function object passed to wrong V8Js instance");
			return V8JS_NULL;
		}

		return v8::Local<v8::Value>::New(isolate, c->v8obj);
	}

	/* If it's a PHP object, wrap it */
	if (ce) {
		return v8js_wrap_object(isolate, ce, value TSRMLS_CC);
	}

	/* Associative PHP arrays cannot be wrapped to JS arrays, convert them to
	 * JS objects and attach all their array keys as properties. */
	return v8js_wrap_array_to_object(isolate, value TSRMLS_CC);
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
