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

#ifndef PHP_V8JS_MACROS_H
#define PHP_V8JS_MACROS_H

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
#define _GLIBCXX_USE_NANOSLEEP 1
#endif

extern "C" {
#include "php.h"
#include "php_v8js.h"
}

#ifdef _WIN32
/* On Windows a symbol COMPILER is defined.  However v8.h has an enum with that
 * name which hence would be broken unless undefined here. */
#undef COMPILER
#endif

#include <v8.h>

#include <chrono>
#include <deque>
#include <thread>

#include <map>
#include <list>
#include <vector>
#include <mutex>

#include "v8js_class.h"
#include "v8js_v8.h"
#include "v8js_timer.h"

#ifndef PATH_MAX
/* Some platforms (Windows among others) don't have a PATH_MAX, for the moment
 * just assume an arbitrary upper bound of 4096 chars.
 * Anyways we should fix (get rid of) the code that uses PATH_MAX as it doesn't
 * even check for buffer overflows.  FIXME */
#define PATH_MAX 4096
#endif

/* V8Js Version */
#define PHP_V8JS_VERSION "0.2.0"

/* Hidden field name used to link JS wrappers with underlying PHP object */
#define PHPJS_OBJECT_KEY "phpjs::object"

/* Helper macros */
#define V8JS_GET_CLASS_NAME(var, obj) \
	v8::String::Utf8Value var(obj->GetConstructorName());

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

/* Convert zval into V8 value */
v8::Handle<v8::Value> zval_to_v8js(zval *, v8::Isolate * TSRMLS_DC);

/* Convert V8 value into zval */
int v8js_to_zval(v8::Handle<v8::Value>, zval *, int, v8::Isolate * TSRMLS_DC);

struct v8js_accessor_ctx
{
    char *variable_name_string;
    uint variable_name_string_len;
    v8::Isolate *isolate;
};

void v8js_accessor_ctx_dtor(v8js_accessor_ctx * TSRMLS_DC);

/* Register accessors into passed object */
void v8js_register_accessors(std::vector<v8js_accessor_ctx*> *accessor_list, v8::Local<v8::FunctionTemplate>, zval *, v8::Isolate * TSRMLS_DC);

/* Resource declaration */

/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(v8js)
  int v8_initialized;
  HashTable *extensions;

  /* Ini globals */
  char *v8_flags; /* V8 command line flags */
  bool use_date; /* Generate JS Date objects instead of PHP DateTime */
  bool use_array_access; /* Convert ArrayAccess, Countable objects to array-like objects */

  // Timer thread globals
  std::deque<v8js_timer_ctx *> timer_stack;
  std::thread *timer_thread;
  std::mutex timer_mutex;
  bool timer_stop;

  bool fatal_error_abort;
ZEND_END_MODULE_GLOBALS(v8js)

extern zend_v8js_globals v8js_globals;

ZEND_EXTERN_MODULE_GLOBALS(v8js)

#ifdef ZTS
# define V8JSG(v) TSRMG(v8js_globals_id, zend_v8js_globals *, v)
#else
# define V8JSG(v) (v8js_globals.v)
#endif

/* Register builtin methods into passed object */
void v8js_register_methods(v8::Handle<v8::ObjectTemplate>, v8js_ctx *c);

#endif	/* PHP_V8JS_MACROS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
