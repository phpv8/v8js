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
#include "v8js_array_access.h"
#include "v8js_exceptions.h"
#include "v8js_generator_export.h"
#include "v8js_object_export.h"
#include "v8js_v8object_class.h"

extern "C" {
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
#include "zend_generators.h"
}

static void v8js_weak_object_callback(const v8::WeakCallbackInfo<zend_object> &data);

/* Callback for PHP methods and functions */
static void v8js_call_php_func(zend_object *object, zend_function *method_ptr, const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();
	v8::Local<v8::Value> return_value = V8JS_NULL;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval fname, retval;
	unsigned int argc = info.Length(), min_num_args = 0, max_num_args = 0;
	char *error;
	zend_ulong error_len;
	unsigned int i;

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	/* Set parameter limits */
	min_num_args = method_ptr->common.required_num_args;
	max_num_args = method_ptr->common.num_args;

	/* Function name to call */
	ZVAL_STR_COPY(&fname, method_ptr->common.function_name);

	/* zend_fcall_info */
	fci.size = sizeof(fci);
#if (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION == 0)
	fci.function_table = &object->ce->function_table;
	fci.symbol_table = NULL;
#endif
	fci.function_name = fname;
	fci.object = object;
	fci.retval = &retval;
	fci.param_count = 0;

	/* Check for passed vs required number of arguments */
	if (argc < min_num_args)
	{
		error_len = spprintf(&error, 0,
			"%s::%s() expects %s %d parameter%s, %d given",
				ZSTR_VAL(object->ce->name),
				ZSTR_VAL(method_ptr->common.function_name),
				min_num_args == max_num_args ? "exactly" : argc < min_num_args ? "at least" : "at most",
				argc < min_num_args ? min_num_args : max_num_args,
				(argc < min_num_args ? min_num_args : max_num_args) == 1 ? "" : "s",
				argc);

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		if (object->ce == zend_ce_closure) {
			zend_string_release(method_ptr->internal_function.function_name);
			efree(method_ptr);
		}
		efree(error);
		info.GetReturnValue().Set(return_value);
		zval_ptr_dtor(&fname);
		return;
	}

	/* Convert parameters passed from V8 */
	if (argc)
	{
		fci.params = (zval *) safe_emalloc(argc, sizeof(zval), 0);
		for (i = 0; i < argc; i++)
		{
			v8::Local<v8::Object> param_object;

			if (info[i]->IsObject() && info[i]->ToObject(v8_context).ToLocal(&param_object) && param_object->InternalFieldCount() == 2)
			{
				/* This is a PHP object, passed to JS and back. */
				zend_object *object = reinterpret_cast<zend_object *>(param_object->GetAlignedPointerFromInternalField(1));
				ZVAL_OBJ(&fci.params[i], object);
				Z_ADDREF_P(&fci.params[i]);
			}
			else
			{
				if (v8js_to_zval(info[i], &fci.params[i], ctx->flags, isolate) == FAILURE)
				{
					error_len = spprintf(&error, 0, "converting parameter #%d passed to %s() failed", i + 1, method_ptr->common.function_name);

					if (error_len > std::numeric_limits<int>::max()) {
						zend_throw_exception(php_ce_v8js_exception,
							"Generated error message length exceeds maximum supported length", 0);
					}
					else {
						return_value = V8JS_THROW(isolate, Error, error, static_cast<int>(error_len));
					}

					efree(error);
					goto failure;
				}
			}

			fci.param_count++;
 		}
	} else {
		fci.params = NULL;
	}

	fci.no_separation = 1;
	info.GetReturnValue().Set(V8JS_NULL);

	{
		isolate->Exit();
		v8::Unlocker unlocker(isolate);

		zend_try {
			/* zend_fcall_info_cache */
#if PHP_VERSION_ID < 70300
			fcc.initialized = 1;
#endif
			fcc.function_handler = method_ptr;
			fcc.calling_scope = object->ce;
			fcc.called_scope = object->ce;
			fcc.object = object;

			zend_call_function(&fci, &fcc);
		}
		zend_catch {
			v8js_terminate_execution(isolate);
			V8JSG(fatal_error_abort) = 1;
		}
		zend_end_try();
	}
	isolate->Enter();

failure:
	/* Cleanup */
	if (argc) {
		for (i = 0; i < fci.param_count; i++) {
			zval_ptr_dtor(&fci.params[i]);
		}
		efree(fci.params);
	}

	if(EG(exception)) {
		if(ctx->flags & V8JS_FLAG_PROPAGATE_PHP_EXCEPTIONS) {
			zval tmp_zv;
			ZVAL_OBJ(&tmp_zv, EG(exception));
			return_value = isolate->ThrowException(zval_to_v8js(&tmp_zv, isolate));
			zend_clear_exception();
		} else {
			v8js_terminate_execution(isolate);
		}
	} else if (Z_TYPE(retval) == IS_OBJECT && Z_OBJ(retval) == object) {
		// special case: "return $this"
		return_value = info.Holder();
	} else {
		return_value = zval_to_v8js(&retval, isolate);
	}

	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&fname);

	info.GetReturnValue().Set(return_value);
}
/* }}} */

/* Callback for PHP methods and functions */
void v8js_php_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Local<v8::Object> self = info.Holder();

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));
	zend_function *method_ptr;

	/* Set method_ptr from v8::External or fetch the closure invoker */
	if (!info.Data().IsEmpty() && info.Data()->IsExternal()) {
		method_ptr = static_cast<zend_function *>(v8::External::Cast(*info.Data())->Value());
	} else {
		method_ptr = zend_get_closure_invoke_method(object);
	}

	return v8js_call_php_func(object, method_ptr, info);
}

/* Callback for PHP constructor calls */
static void v8js_construct_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	info.GetReturnValue().Set(V8JS_UNDEFINED);

	v8::Local<v8::Object> newobj = info.This();
	zval value;

	if (!info.IsConstructCall()) {
		return;
	}

	v8::Local<v8::Array> cons_data = v8::Local<v8::Array>::Cast(info.Data());
	v8::Local<v8::Value> cons_tmpl, cons_ce;

	if (!cons_data->Get(isolate->GetEnteredContext(), 0).ToLocal(&cons_tmpl)
			||!cons_data->Get(isolate->GetEnteredContext(), 1).ToLocal(&cons_ce))
	{
		return;
	}

	v8::Local<v8::External> ext_tmpl = v8::Local<v8::External>::Cast(cons_tmpl);
	v8::Local<v8::External> ext_ce =  v8::Local<v8::External>::Cast(cons_ce);

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	if (info[0]->IsExternal()) {
		// Object created by v8js in v8js_hash_to_jsobj, PHP object passed as v8::External.
		v8::Local<v8::External> php_object = v8::Local<v8::External>::Cast(info[0]);
		zend_object *object = reinterpret_cast<zend_object *>(php_object->Value());
		ZVAL_OBJ(&value, object);

		if(ctx->weak_objects.count(object)) {
			// We already exported this object, hence no need to add another
			// ref, v8 won't give us a second weak-object callback anyways.
			newobj->SetAlignedPointerInInternalField(0, ext_tmpl->Value());
			newobj->SetAlignedPointerInInternalField(1, object);
			return;
		}

		// Increase the reference count of this value because we're storing it internally for use later
		// See https://github.com/phpv8/v8js/issues/6
		Z_ADDREF_P(&value);
	} else {
		// Object created from JavaScript context.  Need to create PHP object first.
		zend_class_entry *ce = static_cast<zend_class_entry *>(ext_ce->Value());
		zend_function *ctor_ptr = ce->constructor;

		// Check access on __construct function, if any
		if (ctor_ptr != NULL && (ctor_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0) {
			info.GetReturnValue().Set(isolate->ThrowException(V8JS_SYM("Call to protected __construct() not allowed")));
			return;
		}

		object_init_ex(&value, ce);

		// Call __construct function
		if (ctor_ptr != NULL) {
			v8js_call_php_func(Z_OBJ(value), ctor_ptr, info);
		}
	}

	newobj->SetAlignedPointerInInternalField(0, ext_tmpl->Value());
	newobj->SetAlignedPointerInInternalField(1, Z_OBJ(value));

	// Since we got to decrease the reference count again, in case v8 garbage collector
	// decides to dispose the JS object, we add a weak persistent handle and register
	// a callback function that removes the reference.
	ctx->weak_objects[Z_OBJ(value)].Reset(isolate, newobj);
	ctx->weak_objects[Z_OBJ(value)].SetWeak(Z_OBJ(value), v8js_weak_object_callback, v8::WeakCallbackType::kParameter);

	// Just tell v8 that we're allocating some external memory
	// (for the moment we just always tell 1k instead of trying to find out actual values)
	isolate->AdjustAmountOfExternalAllocatedMemory(ctx->average_object_size);
}
/* }}} */


static void v8js_weak_object_callback(const v8::WeakCallbackInfo<zend_object> &data) {
	v8::Isolate *isolate = data.GetIsolate();

	zend_object *object = data.GetParameter();
	zval value;
	ZVAL_OBJ(&value, object);
	zval_ptr_dtor(&value);

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	ctx->weak_objects.at(object).Reset();
	ctx->weak_objects.erase(object);

	isolate->AdjustAmountOfExternalAllocatedMemory(-ctx->average_object_size);
}

static void v8js_weak_closure_callback(const v8::WeakCallbackInfo<v8js_function_tmpl_t> &data) {
	v8::Isolate *isolate = data.GetIsolate();

	v8js_function_tmpl_t *persist_tpl_ = data.GetParameter();
	persist_tpl_->Reset();
	delete persist_tpl_;

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

	ctx->weak_closures.at(persist_tpl_).Reset();
	ctx->weak_closures.erase(persist_tpl_);
};

#define IS_MAGIC_FUNC(mname) \
	((ZSTR_LEN(key) == sizeof(mname) - 1) &&		\
	 !strncasecmp(ZSTR_VAL(key), mname, ZSTR_LEN(key)))

#define PHP_V8JS_CALLBACK(isolate, mptr, tmpl)										\
	(v8::FunctionTemplate::New((isolate), v8js_php_callback, v8::External::New((isolate), mptr), v8::Signature::New((isolate), tmpl))->GetFunction(isolate->GetEnteredContext()).ToLocalChecked())


static void v8js_named_property_enumerator(const v8::PropertyCallbackInfo<v8::Array> &info) /* {{{ */
{
	// note: 'special' properties like 'constructor' are not enumerated.
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();

	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Array> result = v8::Array::New(isolate, 0);
	uint32_t result_len = 0;

	zend_class_entry *ce;
	void *ptr;
	HashTable *proptable;
	zend_string *key;
	zend_ulong index;

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));
	ce = object->ce;

	/* enumerate all methods */
	ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->function_table, key, ptr) {
		zend_function *method_ptr = reinterpret_cast<zend_function *>(ptr);
		if ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0) {
			/* Allow only public methods */
			continue;
		}
		if ((method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR)) != 0) {
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

		v8::Local<v8::String> method_name;

		// rename PHP special method names to JS equivalents.
		if (IS_MAGIC_FUNC(ZEND_TOSTRING_FUNC_NAME)) {
			method_name = V8JS_SYM("toString");
		}
		else {
			if (ZSTR_LEN(method_ptr->common.function_name) > std::numeric_limits<int>::max()) {
				zend_throw_exception(php_ce_v8js_exception,
					"Method name length exceeds maximum supported length", 0);
				return;
			}

			method_name = V8JS_STRL(ZSTR_VAL(method_ptr->common.function_name), static_cast<int>(ZSTR_LEN(method_ptr->common.function_name)));
		}

		result->Set(v8_context, result_len++, method_name);
	} ZEND_HASH_FOREACH_END();

	/* enumerate all properties */
	/* Z_OBJPROP uses the get_properties handler */
	zval tmp_zv;
	ZVAL_OBJ(&tmp_zv, object);
	proptable = Z_OBJPROP(tmp_zv);

	ZEND_HASH_FOREACH_KEY(proptable, index, key) {
		if(key) {
			/* skip protected and private members */
			if(ZSTR_VAL(key)[0] == '\0') {
				continue;
			}

			if (ZSTR_LEN(key) + 1 > std::numeric_limits<int>::max()) {
				zend_throw_exception(php_ce_v8js_exception,
					"Object key length exceeds maximum supported length", 0);
				continue;
			}

			// prefix enumerated property names with '$' so they can be
			// dereferenced unambiguously (ie, don't conflict with method
			// names)
			char *prefixed = static_cast<char *>(emalloc(ZSTR_LEN(key) + 2));
			prefixed[0] = '$';
			strncpy(prefixed + 1, ZSTR_VAL(key), ZSTR_LEN(key) + 1);
			result->Set(v8_context, result_len++, V8JS_STRL(prefixed, static_cast<int>(ZSTR_LEN(key) + 1)));
			efree(prefixed);
		} else {
			// even numeric indices are enumerated as strings in JavaScript
			result->Set(v8_context, result_len++, V8JS_FLOAT((double) index)->ToString(v8_context).ToLocalChecked());
		}
	} ZEND_HASH_FOREACH_END();

	/* done */
	info.GetReturnValue().Set(result);
}
/* }}} */

static void v8js_invoke_callback(const v8::FunctionCallbackInfo<v8::Value>& info) /* {{{ */
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();

	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info.Data());
	int argc = info.Length(), i;
	v8::Local<v8::Value> *argv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));
	v8::Local<v8::Value> result;

	for (i=0; i<argc; i++) {
		new(&argv[i]) v8::Local<v8::Value>;
		argv[i] = info[i];
	}

	if (info.IsConstructCall()) {
		v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);

		v8::Local<v8::String> str = self->GetConstructorName()->ToString(v8_context).ToLocalChecked();
		v8::String::Utf8Value str_value(isolate, str);
		zend_string *constructor_name = zend_string_init(ToCString(str_value), str_value.length(), 0);
		zend_class_entry *ce = zend_lookup_class(constructor_name);
		zend_string_release(constructor_name);

		v8::Local<v8::FunctionTemplate> new_tpl;
		new_tpl = v8::Local<v8::FunctionTemplate>::New
			(isolate, ctx->template_cache.at(ce->name));

		v8::Local<v8::Function> fn;
		if (!new_tpl->GetFunction(v8_context).ToLocal(&fn) || !fn->NewInstance(v8_context, argc, argv).ToLocal(&result))
		{
			result = V8JS_UNDEFINED;
		}
	} else {
		cb->Call(v8_context, self, argc, argv).ToLocal(&result);
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
	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();

	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Value> return_value = V8JS_NULL;

	char *error;
	size_t error_len;

	zend_class_entry *ce;
	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));
	ce = object->ce;

	// first arg is method name, second arg is array of args.
	if (info.Length() < 2) {
		error_len = spprintf(&error, 0,
			"%s::__call expects 2 parameters, %d given",
			ce->name, (int) info.Length());

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	if (!info[1]->IsArray()) {
		error_len = spprintf(&error, 0,
			"%s::__call expects 2nd parameter to be an array",
			ce->name);

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	v8::Local<v8::Array> args = v8::Local<v8::Array>::Cast(info[1]);
	if (args->Length() > 1000000) {
		// prevent overflow, since args->Length() is a uint32_t and args
		// in the Function->Call method below is a (signed) int.
		error_len = spprintf(&error, 0,
			"%s::__call expects fewer than a million arguments",
			ce->name);

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	v8::MaybeLocal<v8::String> str = info[0]->ToString(v8_context);

	if (str.IsEmpty())
	{
		error_len = spprintf(&error, 0,
			"%s::__call expect 1st parameter to be valid function name, toString() invocation failed.",
			ce->name);

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	v8::String::Utf8Value str_value(isolate, str.ToLocalChecked());
	const char *method_c_name = ToCString(str_value);
	zend_string *method_name = zend_string_init(method_c_name, str_value.length(), 0);

	// okay, look up the method name and manually invoke it.
	const zend_object_handlers *h = object->handlers;
	zend_function *method_ptr = h->get_method(&object, method_name, NULL);
	zend_string_release(method_name);

	if (method_ptr == NULL ||
		(method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) == 0 ||
		(method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR)) != 0) {
		error_len = spprintf(&error, 0,
			"%s::__call to %s method %s", ce->name,
			(method_ptr == NULL) ? "undefined" : "non-public", method_name);

		if (error_len > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Generated error message length exceeds maximum supported length", 0);
		}
		else {
			return_value = V8JS_THROW(isolate, TypeError, error, static_cast<int>(error_len));
		}

		efree(error);
		info.GetReturnValue().Set(return_value);
		return;
	}

	v8::Local<v8::FunctionTemplate> tmpl =
		v8::Local<v8::FunctionTemplate>::New
			(isolate, *reinterpret_cast<v8js_function_tmpl_t *>(self->GetAlignedPointerFromInternalField(0)));
	// use v8js_php_callback to actually execute the method
	v8::Local<v8::Function> cb = PHP_V8JS_CALLBACK(isolate, method_ptr, tmpl);
	uint32_t i, argc = args->Length();
	v8::Local<v8::Value> *argv = static_cast<v8::Local<v8::Value> *>(alloca(sizeof(v8::Local<v8::Value>) * argc));
	for (i=0; i<argc; i++) {
		new(&argv[i]) v8::Local<v8::Value>;
		args->Get(v8_context, i).ToLocal(&argv[i]);
	}
	cb->Call(v8_context, info.This(), (int) argc, argv).ToLocal(&return_value);
	info.GetReturnValue().Set(return_value);
}
/* }}} */

/* This method handles named property and method get/set/query/delete. */
template<typename T>
v8::Local<v8::Value> v8js_named_property_callback(v8::Local<v8::Name> property_name, const v8::PropertyCallbackInfo<T> &info, property_op_t callback_type, v8::Local<v8::Value> set_value) /* {{{ */
{
	v8::Local<v8::String> property = v8::Local<v8::String>::Cast(property_name);

	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> v8_context = isolate->GetEnteredContext();
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::String::Utf8Value cstr(isolate, property);
	const char *name = ToCString(cstr);
	uint name_len = cstr.length();
	char *lower = estrndup(name, name_len);
	zend_string *method_name;

	v8::Local<v8::Object> self = info.Holder();
	v8::Local<v8::Value> ret_value;
	v8::Local<v8::Function> cb;

	zend_class_entry *scope, *ce;
	zend_function *method_ptr = NULL;
	zval php_value;

	zend_object *object = reinterpret_cast<zend_object *>(self->GetAlignedPointerFromInternalField(1));
	zval zobject;
	ZVAL_OBJ(&zobject, object);

	v8js_function_tmpl_t *tmpl_ptr = reinterpret_cast<v8js_function_tmpl_t *>(self->GetAlignedPointerFromInternalField(0));
	v8::Local<v8::FunctionTemplate> tmpl = v8::Local<v8::FunctionTemplate>::New(isolate, *tmpl_ptr);
	ce = scope = object->ce;

	/* First, check the (case-insensitive) method table */
	php_strtolower(lower, name_len);
	method_name = zend_string_init(lower, name_len, 0);

	// toString() -> __tostring()
	if (name_len == 8 && strcmp(name, "toString") == 0) {
		zend_string_release(method_name);
		method_name = zend_string_init(ZEND_TOSTRING_FUNC_NAME, sizeof(ZEND_TOSTRING_FUNC_NAME) - 1, 0);
	}

	bool is_constructor = (name_len == 11 && strcmp(name, "constructor") == 0);
	bool is_magic_call = (ZSTR_LEN(method_name) == 6 && strcmp(ZSTR_VAL(method_name), "__call") == 0);

	if (is_constructor ||
		(name[0] != '$' /* leading '$' means property, not method */ &&
		 (method_ptr = reinterpret_cast<zend_function *>
		  (zend_hash_find_ptr(&ce->function_table, method_name))) &&
		 ((method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) != 0) && /* Allow only public methods */
		 ((method_ptr->common.fn_flags & (ZEND_ACC_CTOR|ZEND_ACC_DTOR)) == 0) /* no __construct, __destruct(), or __clone() functions */
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
					v8::Local<v8::FunctionTemplate> ft;
					try {
						ft = v8::Local<v8::FunctionTemplate>::New
							(isolate, ctx->call_impls.at(tmpl_ptr));
					}
					catch (const std::out_of_range &) {
						ft = v8::FunctionTemplate::New(isolate,
								v8js_fake_call_impl, V8JS_NULL,
								v8::Signature::New(isolate, tmpl));
						v8js_function_tmpl_t *persistent_ft = &ctx->call_impls[tmpl_ptr];
						persistent_ft->Reset(isolate, ft);
					}

					v8::Local<v8::Function> fn;
					if (ft->GetFunction(v8_context).ToLocal(&fn))
					{
						fn->SetName(property);
						ret_value = fn;
					}
				} else {
					v8::Local<v8::FunctionTemplate> ft;
					try {
						ft = v8::Local<v8::FunctionTemplate>::New
							(isolate, ctx->method_tmpls.at(std::make_pair(ce, method_ptr)));
					}
					catch (const std::out_of_range &) {
						ft = v8::FunctionTemplate::New(isolate, v8js_php_callback,
								v8::External::New((isolate), method_ptr),
								v8::Signature::New((isolate), tmpl));
						v8js_function_tmpl_t *persistent_ft = &ctx->method_tmpls[std::make_pair(ce, method_ptr)];
						persistent_ft->Reset(isolate, ft);
					}
					ft->GetFunction(v8_context).ToLocal(&ret_value);
				}
			}
		} else if (callback_type == V8JS_PROP_QUERY) {
			ret_value = V8JS_UINT(v8::ReadOnly|v8::DontDelete);
		} else if (callback_type == V8JS_PROP_SETTER) {
			ret_value = set_value; // lie.  this field is read-only.
		} else if (callback_type == V8JS_PROP_DELETER) {
			ret_value = V8JS_BOOL(false);
		} else {
			/* shouldn't reach here! but bail safely */
			ret_value = v8::Local<v8::Value>();
		}
	} else {
		if (name[0]=='$') {
			// this is a property (not a method)
			name++; name_len--;
		}

		zval zname;
		ZVAL_STRINGL(&zname, name, name_len);

		if (callback_type == V8JS_PROP_GETTER) {
			/* Nope, not a method -- must be a (case-sensitive) property */
			zend_property_info *property_info = zend_get_property_info(ce, Z_STR(zname), 1);

			if(!property_info ||
			   (property_info != ZEND_WRONG_PROPERTY_INFO &&
				property_info->flags & ZEND_ACC_PUBLIC)) {
				zval *property_val = zend_read_property(NULL, &zobject, name, name_len, true, &php_value);
				// special case uninitialized_zval_ptr and return an empty value
				// (indicating that we don't intercept this property) if the
				// property doesn't exist.
				if (property_val == &EG(uninitialized_zval)) {
					ret_value = v8::Local<v8::Value>();
				} else {
					// wrap it
					ret_value = zval_to_v8js(property_val, isolate);
					/* We don't own the reference to php_value... unless the
					 * returned refcount was 0, in which case the below code
					 * will free it. */
					zval_add_ref(property_val);
					zval_ptr_dtor(property_val);
				}
			}
			else if (ce->__get
					 /* Allow only public methods */
					 && ((ce->__get->common.fn_flags & ZEND_ACC_PUBLIC) != 0)) {
				/* Okay, let's call __get. */
				zend_call_method_with_1_params(&zobject, ce, &ce->__get, ZEND_GET_FUNC_NAME, &php_value, &zname);
				ret_value = zval_to_v8js(&php_value, isolate);
				zval_ptr_dtor(&php_value);
			}

		} else if (callback_type == V8JS_PROP_SETTER) {
			if (v8js_to_zval(set_value, &php_value, ctx->flags, isolate) != SUCCESS) {
				ret_value = v8::Local<v8::Value>();
			}
			else {
				zend_property_info *property_info = zend_get_property_info(ce, Z_STR(zname), 1);

				if(!property_info ||
				   (property_info != ZEND_WRONG_PROPERTY_INFO &&
					property_info->flags & ZEND_ACC_PUBLIC)) {
					zend_update_property(scope, &zobject, name, name_len, &php_value);
					ret_value = set_value;
				}
				else if (ce->__set
						 /* Allow only public methods */
						 && ((ce->__set->common.fn_flags & ZEND_ACC_PUBLIC) != 0)) {
					/* Okay, let's call __set. */
					zval php_ret_value;
					zend_call_method_with_2_params(&zobject, ce, &ce->__set, ZEND_SET_FUNC_NAME, &php_ret_value, &zname, &php_value);
					ret_value = zval_to_v8js(&php_ret_value, isolate);
					zval_ptr_dtor(&php_ret_value);
				}
			}

			// if PHP wanted to hold on to this value, update_property would
			// have bumped the refcount
			zval_ptr_dtor(&php_value);
		} else if (callback_type == V8JS_PROP_QUERY ||
				   callback_type == V8JS_PROP_DELETER) {
			const zend_object_handlers *h = object->handlers;

			if (callback_type == V8JS_PROP_QUERY) {
				if (h->has_property(&zobject, &zname, 0, NULL)) {
					ret_value = V8JS_UINT(v8::None);
				} else {
					ret_value = v8::Local<v8::Value>(); // empty handle
				}
			} else {
				zend_property_info *property_info = zend_get_property_info(ce, Z_STR(zname), 1);

				if(!property_info ||
				   (property_info != ZEND_WRONG_PROPERTY_INFO &&
					property_info->flags & ZEND_ACC_PUBLIC)) {
					h->unset_property(&zobject, &zname, NULL);
					ret_value = V8JS_TRUE();
				}
				else {
					ret_value = v8::Local<v8::Value>(); // empty handle
				}
			}
		} else {
			/* shouldn't reach here! but bail safely */
			ret_value = v8::Local<v8::Value>();
		}

		zval_ptr_dtor(&zname);
	}

	zend_string_release(method_name);
	efree(lower);
	return ret_value;
}
/* }}} */

static void v8js_named_property_getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info) /* {{{ */
{
	info.GetReturnValue().Set(v8js_named_property_callback(property, info, V8JS_PROP_GETTER));
}
/* }}} */

static void v8js_named_property_setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info) /* {{{ */
{
	info.GetReturnValue().Set(v8js_named_property_callback(property, info, V8JS_PROP_SETTER, value));
}
/* }}} */

static void v8js_named_property_query(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Integer> &info) /* {{{ */
{
	v8::Local<v8::Value> r = v8js_named_property_callback(property, info, V8JS_PROP_QUERY);
	if (r.IsEmpty()) {
		return;
	}

	v8::Isolate *isolate = info.GetIsolate();
	v8::MaybeLocal<v8::Integer> value = r->ToInteger(isolate->GetEnteredContext());
	if (!value.IsEmpty()) {
		info.GetReturnValue().Set(value.ToLocalChecked());
	}
}
/* }}} */

static void v8js_named_property_deleter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean> &info) /* {{{ */
{
	v8::Local<v8::Value> r = v8js_named_property_callback(property, info, V8JS_PROP_DELETER);
	if (r.IsEmpty()) {
		return;
	}

	v8::Isolate *isolate = info.GetIsolate();
	v8::MaybeLocal<v8::Boolean> value = r->ToBoolean(isolate->GetEnteredContext());
	if (!value.IsEmpty()) {
		info.GetReturnValue().Set(value.ToLocalChecked());
	}
}
/* }}} */



static v8::MaybeLocal<v8::Object> v8js_wrap_object(v8::Isolate *isolate, zend_class_entry *ce, zval *value) /* {{{ */
{
	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, ctx->context);
	v8::Local<v8::FunctionTemplate> new_tpl;
	v8js_function_tmpl_t *persist_tpl_;

	try {
		new_tpl = v8::Local<v8::FunctionTemplate>::New
			(isolate, ctx->template_cache.at(ce->name));
	}
	catch (const std::out_of_range &) {
		/* No cached v8::FunctionTemplate available as of yet, create one. */
		new_tpl = v8::FunctionTemplate::New(isolate, 0);

		if (ZSTR_LEN(ce->name) > std::numeric_limits<int>::max()) {
			zend_throw_exception(php_ce_v8js_exception,
				"Class name length exceeds maximum supported length", 0);
			return v8::Local<v8::Object>();
		}

		new_tpl->SetClassName(V8JS_STRL(ZSTR_VAL(ce->name), static_cast<int>(ZSTR_LEN(ce->name))));
		new_tpl->InstanceTemplate()->SetInternalFieldCount(2);

		if (ce == zend_ce_closure) {
			/* Got a closure, mustn't cache ... */
			persist_tpl_ = new v8js_function_tmpl_t(isolate, new_tpl);
			/* We'll free persist_tpl_ via v8js_weak_closure_callback, below */
			new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(v8js_php_callback);
		} else {
			/* Add new v8::FunctionTemplate to tpl_map, as long as it is not a closure. */
			persist_tpl_ = &ctx->template_cache[ce->name];
			persist_tpl_->Reset(isolate, new_tpl);
			/* We'll free persist_tpl_ when template_cache is destroyed */

			v8::Local<v8::ObjectTemplate> inst_tpl = new_tpl->InstanceTemplate();
			v8::GenericNamedPropertyGetterCallback getter = v8js_named_property_getter;
			v8::GenericNamedPropertyEnumeratorCallback enumerator = v8js_named_property_enumerator;

			/* Check for ArrayAccess object */
			if (V8JSG(use_array_access) && ce) {
				bool has_array_access = false;
				bool has_countable = false;

				for (unsigned int i = 0; i < ce->num_interfaces; i ++) {
					if (strcmp (ZSTR_VAL(ce->interfaces[i]->name), "ArrayAccess") == 0) {
						has_array_access = true;
					}
					else if (strcmp (ZSTR_VAL(ce->interfaces[i]->name), "Countable") == 0) {
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
			inst_tpl->SetHandler(v8::NamedPropertyHandlerConfiguration
				(getter, /* getter */
				 v8js_named_property_setter, /* setter */
				 v8js_named_property_query, /* query */
				 v8js_named_property_deleter, /* deleter */
				 enumerator, /* enumerator */
				 V8JS_NULL /* data */
				 ));
			// add __invoke() handler
			zend_string *invoke_str = zend_string_init
				(ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME) - 1, 0);
			zend_function *invoke_method_ptr = reinterpret_cast<zend_function *>
				(zend_hash_find_ptr(&ce->function_table, invoke_str));
			if (invoke_method_ptr &&
				invoke_method_ptr->common.fn_flags & ZEND_ACC_PUBLIC) {
				new_tpl->InstanceTemplate()->SetCallAsFunctionHandler(v8js_invoke_callback, PHP_V8JS_CALLBACK(isolate, invoke_method_ptr, new_tpl));
			}
			zend_string_release(invoke_str);
		}
		v8::Local<v8::Array> call_handler_data = v8::Array::New(isolate, 2);
		call_handler_data->Set(v8_context, 0, v8::External::New(isolate, persist_tpl_));
		call_handler_data->Set(v8_context, 1, v8::External::New(isolate, ce));
		new_tpl->SetCallHandler(v8js_construct_callback, call_handler_data);
	}

	// Create v8 wrapper object
	v8::Local<v8::Value> external = v8::External::New(isolate, Z_OBJ_P(value));

	v8::Local<v8::Function> constr;
	if (!new_tpl->GetFunction(v8_context).ToLocal(&constr)) {
		return v8::MaybeLocal<v8::Object>();
	}

	v8::MaybeLocal<v8::Object> newobj = constr->NewInstance(v8_context, 1, &external);

	if (ce == zend_ce_closure && !newobj.IsEmpty()) {
		// free uncached function template when object is freed
		ctx->weak_closures[persist_tpl_].Reset(isolate, newobj.ToLocalChecked());
		ctx->weak_closures[persist_tpl_].SetWeak(persist_tpl_, v8js_weak_closure_callback, v8::WeakCallbackType::kParameter);
	}

	return newobj;
}
/* }}} */


static v8::Local<v8::Object> v8js_wrap_array_to_object(v8::Isolate *isolate, zval *value) /* {{{ */
{
	int i;
	zend_string *key;
	zend_ulong index;

	v8js_ctx *ctx = (v8js_ctx *) isolate->GetData(0);
	v8::Local<v8::Context> v8_context = v8::Local<v8::Context>::New(isolate, ctx->context);
	v8::Local<v8::FunctionTemplate> new_tpl;

	if(ctx->array_tmpl.IsEmpty()) {
		new_tpl = v8::FunctionTemplate::New(isolate, 0);

		/* Call it Array, but it is not a native array, especially it doesn't have
		 * have the typical Array.prototype functions. */
		new_tpl->SetClassName(V8JS_SYM("Array"));

		/* Store for later re-use */
		ctx->array_tmpl.Reset(isolate, new_tpl);
	}
	else {
		new_tpl = v8::Local<v8::FunctionTemplate>::New(isolate, ctx->array_tmpl);
	}

	v8::Local<v8::Object> newobj = new_tpl->InstanceTemplate()->NewInstance(v8_context).ToLocalChecked();

	HashTable *myht = HASH_OF(value);
	i = myht ? zend_hash_num_elements(myht) : 0;

	if (i > 0)
	{
		zval *data;

#if PHP_VERSION_ID >= 70300
		if (myht && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
#else
		if (myht) {
#endif
			GC_PROTECT_RECURSION(myht);
		}

		ZEND_HASH_FOREACH_KEY_VAL(myht, index, key, data) {

			if (key) {
				if (ZSTR_VAL(key)[0] == '\0' && Z_TYPE_P(value) == IS_OBJECT) {
					/* Skip protected and private members. */
					continue;
				}

				if (ZSTR_LEN(key) > std::numeric_limits<int>::max()) {
					zend_throw_exception(php_ce_v8js_exception,
						"Object key length exceeds maximum supported length", 0);
					continue;
				}

				newobj->Set(v8_context, V8JS_STRL(ZSTR_VAL(key), static_cast<int>(ZSTR_LEN(key))),
					zval_to_v8js(data, isolate));
			} else {
				if (index > std::numeric_limits<uint32_t>::max()) {
					zend_throw_exception(php_ce_v8js_exception,
						"Array index exceeds maximum supported bound", 0);
					continue;
				}

				newobj->Set(v8_context, static_cast<uint32_t>(index), zval_to_v8js(data, isolate));
			}

		} ZEND_HASH_FOREACH_END();

#if PHP_VERSION_ID >= 70300
		if (myht && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
#else
		if (myht) {
#endif
			GC_UNPROTECT_RECURSION(myht);
		}

	}

	return newobj;
}
/* }}} */


v8::Local<v8::Value> v8js_hash_to_jsobj(zval *value, v8::Isolate *isolate) /* {{{ */
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
#if PHP_VERSION_ID >= 70300
	if (myht && GC_IS_RECURSIVE(myht)) {
#else
	if (myht && ZEND_HASH_GET_APPLY_COUNT(myht) > 1) {
#endif
		return V8JS_NULL;
	}

	/* Special case, passing back object originating from JS to JS */
	if (ce == php_ce_v8function || ce == php_ce_v8object || ce == php_ce_v8generator) {
		v8js_v8object *c = Z_V8JS_V8OBJECT_OBJ_P(value);

		if(isolate != c->ctx->isolate) {
			php_error_docref(NULL, E_WARNING, "V8Function object passed to wrong V8Js instance");
			return V8JS_NULL;
		}

		return v8::Local<v8::Value>::New(isolate, c->v8obj);
	}

	/* If it's a PHP object, wrap it */
	if (ce) {
		v8::MaybeLocal<v8::Object> wrapped_object = v8js_wrap_object(isolate, ce, value);

		if (wrapped_object.IsEmpty()) {
			return V8JS_UNDEFINED;
		}

		if (ce == zend_ce_generator) {
			/* Wrap PHP Generator object in a wrapper function that provides
			 * ES6 style behaviour. */
			return v8js_wrap_generator(isolate, wrapped_object.ToLocalChecked());
		}

		return wrapped_object.ToLocalChecked();
	}

	/* Associative PHP arrays cannot be wrapped to JS arrays, convert them to
	 * JS objects and attach all their array keys as properties. */
	return v8js_wrap_array_to_object(isolate, value);
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
