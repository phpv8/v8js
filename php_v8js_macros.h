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

#ifndef PHP_V8JS_MACROS_H
#define PHP_V8JS_MACROS_H

extern "C" {
#include "php.h"
#include "php_v8js.h"
}

#include <v8.h>

#include <chrono>
#include <stack>
#include <thread>

#include <map>
#include <vector>
#include <mutex>

/* V8Js Version */
#define V8JS_VERSION "0.1.3"

/* Helper macros */
#define V8JS_SYM(v)			v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, sizeof(v) - 1)
#define V8JS_SYML(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, l)
#define V8JS_STR(v)			v8::String::NewFromUtf8(isolate, v)
#define V8JS_STRL(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kNormalString, l)
#define V8JS_INT(v)			v8::Integer::New(v, isolate)
#define V8JS_FLOAT(v)		v8::Number::New(isolate, v)
#define V8JS_BOOL(v)		((v)?v8::True(isolate):v8::False(isolate))
#define V8JS_NULL			v8::Null(isolate)
#define V8JS_UNDEFINED		v8::Undefined(isolate)
#define V8JS_MN(name)		v8js_method_##name
#define V8JS_METHOD(name)	void V8JS_MN(name)(const v8::FunctionCallbackInfo<v8::Value>& info)
#define V8JS_THROW(type, message, message_len)	v8::ThrowException(v8::Exception::type(V8JS_STRL(message, message_len)))
#define V8JS_GLOBAL			v8::Context::GetCurrent()->Global()

#if PHP_V8_API_VERSION < 3022000
/* CopyablePersistentTraits is only part of V8 from 3.22.0 on,
   to be compatible with lower versions add our own (compatible) version. */
namespace v8 {
	template<class T>
	struct CopyablePersistentTraits {
		typedef Persistent<T, CopyablePersistentTraits<T> > CopyablePersistent;
		static const bool kResetInDestructor = true;
		template<class S, class M>
#if PHP_V8_API_VERSION >= 3021015
		static V8_INLINE void Copy(const Persistent<S, M>& source,
								   CopyablePersistent* dest)
#else
		V8_INLINE(static void Copy(const Persistent<S, M>& source,
								   CopyablePersistent* dest))
#endif
		{
			// do nothing, just allow copy
		}
	};
}
#endif

/* Abbreviate long type names */
typedef v8::Persistent<v8::FunctionTemplate, v8::CopyablePersistentTraits<v8::FunctionTemplate> > v8js_tmpl_t;

/* Hidden field name used to link JS wrappers with underlying PHP object */
#define PHPJS_OBJECT_KEY "phpjs::object"

/* Helper macros */
#if PHP_V8_API_VERSION < 2005009
# define V8JS_GET_CLASS_NAME(var, obj) \
	/* Hack to prevent calling possibly set user-defined __toString() messing class name */ \
	v8::Local<v8::Function> constructor = v8::Local<v8::Function>::Cast(obj->Get(V8JS_SYM("constructor"))); \
	v8::String::Utf8Value var(constructor->GetName());
#else
# define V8JS_GET_CLASS_NAME(var, obj) \
	v8::String::Utf8Value var(obj->GetConstructorName());
#endif

#if ZEND_MODULE_API_NO >= 20100409
# define ZEND_HASH_KEY_DC , const zend_literal *key
# define ZEND_HASH_KEY_CC , key
# define ZEND_HASH_KEY_NULL , NULL
#else
# define ZEND_HASH_KEY_DC
# define ZEND_HASH_KEY_CC
# define ZEND_HASH_KEY_NULL
#endif

/* method signatures of zend_update_property and zend_read_property were
 * declared as 'char *' instead of 'const char *' before PHP 5.4 */
#if ZEND_MODULE_API_NO >= 20100525
# define V8JS_CONST
#else
# define V8JS_CONST (char *)
#endif

/* Global flags */
#define V8JS_GLOBAL_SET_FLAGS(flags)	V8JS_GLOBAL->SetHiddenValue(V8JS_SYM("__php_flags__"), V8JS_INT(flags))
#define V8JS_GLOBAL_GET_FLAGS()			V8JS_GLOBAL->GetHiddenValue(V8JS_SYM("__php_flags__"))->IntegerValue();

/* Options */
#define V8JS_FLAG_NONE			(1<<0)
#define V8JS_FLAG_FORCE_ARRAY	(1<<1)

#define V8JS_DEBUG_AUTO_BREAK_NEVER		0
#define V8JS_DEBUG_AUTO_BREAK_ONCE		1
#define V8JS_DEBUG_AUTO_BREAK_ALWAYS	2

/* Extracts a C string from a V8 Utf8Value. */
static inline const char * ToCString(const v8::String::Utf8Value &value) /* {{{ */
{
	return *value ? *value : "<string conversion failed>";
}
/* }}} */

/* Extern Class entries */
extern zend_class_entry *php_ce_v8_object;
extern zend_class_entry *php_ce_v8_function;

/* Create PHP V8 object */
void php_v8js_create_v8(zval *, v8::Handle<v8::Value>, int, v8::Isolate * TSRMLS_DC);

/* Fetch V8 object properties */
int php_v8js_v8_get_properties_hash(v8::Handle<v8::Value>, HashTable *, int, v8::Isolate * TSRMLS_DC);

/* Convert zval into V8 value */
v8::Handle<v8::Value> zval_to_v8js(zval *, v8::Isolate * TSRMLS_DC);

/* Convert V8 value into zval */
int v8js_to_zval(v8::Handle<v8::Value>, zval *, int, v8::Isolate * TSRMLS_DC);

/* Register accessors into passed object */
void php_v8js_register_accessors(v8::Local<v8::ObjectTemplate>, zval *, v8::Isolate * TSRMLS_DC);

/* {{{ Context container */
struct php_v8js_ctx {
  zend_object std;
  v8::Persistent<v8::String> object_name;
  v8::Persistent<v8::Context> context;
  zend_bool report_uncaught;
  zval *pending_exception;
  int in_execution;
  v8::Isolate *isolate;
  bool time_limit_hit;
  bool memory_limit_hit;
  v8::Persistent<v8::FunctionTemplate> global_template;
  zval *module_loader;
  std::vector<char *> modules_stack;
  std::vector<char *> modules_base;
  std::map<const char *,v8js_tmpl_t> template_cache;
#ifdef ZTS
  void ***zts_ctx;
#endif
};
/* }}} */

#ifdef ZTS
# define V8JS_TSRMLS_FETCH() TSRMLS_FETCH_FROM_CTX(((php_v8js_ctx *) isolate->GetData())->zts_ctx);
#else
# define V8JS_TSRMLS_FETCH()
#endif

// Timer context
struct php_v8js_timer_ctx
{
  long time_limit;
  long memory_limit;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
  php_v8js_ctx *v8js_ctx;
};

/* {{{ Object container */
struct php_v8js_object {
	zend_object std;
	v8::Persistent<v8::Value> v8obj;
	int flags;
	v8::Isolate *isolate;
};
/* }}} */


/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(v8js)
  int v8_initialized;
  HashTable *extensions;
  int disposed_contexts; /* Disposed contexts since last time V8 did GC */

  /* Ini globals */
  char *v8_flags; /* V8 command line flags */
  int max_disposed_contexts; /* Max disposed context allowed before forcing V8 GC */

  // Timer thread globals
  std::stack<php_v8js_timer_ctx *> timer_stack;
  std::thread *timer_thread;
  std::mutex timer_mutex;
  bool timer_stop;

  std::map<char *, v8::Handle<v8::Object> > modules_loaded;
ZEND_END_MODULE_GLOBALS(v8js)

extern zend_v8js_globals v8js_globals;

ZEND_EXTERN_MODULE_GLOBALS(v8js)

#ifdef ZTS
# define V8JSG(v) TSRMG(v8js_globals_id, zend_v8js_globals *, v)
#else
# define V8JSG(v) (v8js_globals.v)
#endif

/* Register builtin methods into passed object */
void php_v8js_register_methods(v8::Handle<v8::ObjectTemplate>, php_v8js_ctx *c);

#endif	/* PHP_V8JS_MACROS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
