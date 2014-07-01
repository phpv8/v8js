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

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
#define _GLIBCXX_USE_NANOSLEEP 1
#endif

extern "C" {
#include "php.h"
#include "php_v8js.h"
}

#include <v8.h>

#include <chrono>
#include <stack>
#include <thread>

#include <map>
#include <list>
#include <vector>
#include <mutex>

/* V8Js Version */
#define V8JS_VERSION "0.1.5"

/* V8 from 3.23.12 has most v8::Anything::New constructors expect isolates
   as their first argument.  Older versions don't provide these. */
#if PHP_V8_API_VERSION < 3023012
#define V8JS_NEW(t, i, v...) t::New(v)
#else
#define V8JS_NEW(t, i, v...) t::New(i, v)
#endif

/* Helper macros */
#define V8JS_SYM(v)			v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, sizeof(v) - 1)
#define V8JS_SYML(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kInternalizedString, l)
#define V8JS_STR(v)			v8::String::NewFromUtf8(isolate, v)
#define V8JS_STRL(v, l)		v8::String::NewFromUtf8(isolate, v, v8::String::kNormalString, l)

#if PHP_V8_API_VERSION < 3024010
#define V8JS_INT(v)			v8::Integer::New(v, isolate)
#define V8JS_UINT(v)		v8::Integer::NewFromUnsigned(v, isolate)
#else
#define V8JS_INT(v)			v8::Integer::New(isolate, v)
#define V8JS_UINT(v)		v8::Integer::NewFromUnsigned(isolate, v)
#endif

#define V8JS_FLOAT(v)		v8::Number::New(isolate, v)
#define V8JS_BOOL(v)		((v)?v8::True(isolate):v8::False(isolate))

#if PHP_V8_API_VERSION <= 3023012
#define V8JS_DATE(v)		v8::Date::New(v)
#else
#define V8JS_DATE(v)		v8::Date::New(isolate, v)
#endif

#define V8JS_NULL			v8::Null(isolate)
#define V8JS_UNDEFINED		v8::Undefined(isolate)
#define V8JS_MN(name)		v8js_method_##name
#define V8JS_METHOD(name)	void V8JS_MN(name)(const v8::FunctionCallbackInfo<v8::Value>& info)
#define V8JS_THROW(isolate, type, message, message_len)	(isolate)->ThrowException(v8::Exception::type(V8JS_STRL(message, message_len)))
#define V8JS_GLOBAL(isolate)			((isolate)->GetCurrentContext()->Global())

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
typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> > v8js_persistent_obj_t;

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
#define V8JS_GLOBAL_SET_FLAGS(isolate,flags)	V8JS_GLOBAL(isolate)->SetHiddenValue(V8JS_SYM("__php_flags__"), V8JS_INT(flags))
#define V8JS_GLOBAL_GET_FLAGS(isolate)			V8JS_GLOBAL(isolate)->GetHiddenValue(V8JS_SYM("__php_flags__"))->IntegerValue();

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

struct php_v8js_accessor_ctx
{
    char *variable_name_string;
    uint variable_name_string_len;
    v8::Isolate *isolate;
};

void php_v8js_accessor_ctx_dtor(php_v8js_accessor_ctx * TSRMLS_DC);

/* Register accessors into passed object */
void php_v8js_register_accessors(std::vector<php_v8js_accessor_ctx*> *accessor_list, v8::Local<v8::FunctionTemplate>, zval *, v8::Isolate * TSRMLS_DC);

struct php_v8js_object;

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

  std::map<zval *, v8js_persistent_obj_t> weak_objects;
  std::map<v8js_tmpl_t *, v8js_persistent_obj_t> weak_closures;

  std::list<php_v8js_object *> php_v8js_objects;

  std::vector<php_v8js_accessor_ctx *> accessor_list;
  char *tz;
#ifdef ZTS
  void ***zts_ctx;
#endif
};
/* }}} */

#ifdef ZTS
# if PHP_V8_API_VERSION <= 3023008
   /* Until V8 3.23.8 Isolate could only take one external pointer. */
#  define V8JS_TSRMLS_FETCH() TSRMLS_FETCH_FROM_CTX(((php_v8js_ctx *) isolate->GetData())->zts_ctx);
# else
#  define V8JS_TSRMLS_FETCH() TSRMLS_FETCH_FROM_CTX(((php_v8js_ctx *) isolate->GetData(0))->zts_ctx);
# endif
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
  bool killed;
};

/* {{{ Object container */
struct php_v8js_object {
	zend_object std;
	v8::Persistent<v8::Value> v8obj;
	int flags;
	struct php_v8js_ctx *ctx;
	HashTable *properties;
};
/* }}} */

/* Resource declaration */

/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(v8js)
  int v8_initialized;
  HashTable *extensions;

  /* Ini globals */
  char *v8_flags; /* V8 command line flags */
  bool use_date; /* Generate JS Date objects instead of PHP DateTime */

  // Timer thread globals
  std::stack<php_v8js_timer_ctx *> timer_stack;
  std::thread *timer_thread;
  std::mutex timer_mutex;
  bool timer_stop;

  std::map<char *, v8::Handle<v8::Object> > modules_loaded;

  // fatal error unwinding
  bool fatal_error_abort;
  int error_num;
  char *error_message;
  jmp_buf *unwind_env;
  void (*old_error_handler)(int, const char *, const uint, const char*, va_list);
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

typedef struct _php_v8js_script {
	char *name;
	v8::Isolate *isolate;	
	v8::Persistent<v8::Script, v8::CopyablePersistentTraits<v8::Script>> *script;
} php_v8js_script;

static void php_v8js_script_free(php_v8js_script *res, bool dispose_persistent);

#endif	/* PHP_V8JS_MACROS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
