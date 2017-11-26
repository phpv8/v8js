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

#ifndef PHP_V8JS_MACROS_H
#define PHP_V8JS_MACROS_H

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
#define _GLIBCXX_USE_NANOSLEEP 1
#endif

#include <chrono>
#include <deque>
#include <thread>

#include <map>
#include <list>
#include <vector>
#include <mutex>

#include <cmath>
#ifndef isnan
/* php.h requires the isnan() macro, which is removed by c++ <cmath> header,
 * work around: re-define the macro to std::isnan function */
#define isnan(a) std::isnan(a)

/* likewise isfinite */
#define isfinite(a) std::isfinite(a)
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

#include "v8js_class.h"
#include "v8js_v8.h"

#ifndef PATH_MAX
/* Some platforms (Windows among others) don't have a PATH_MAX, for the moment
 * just assume an arbitrary upper bound of 4096 chars.
 * Anyways we should fix (get rid of) the code that uses PATH_MAX as it doesn't
 * even check for buffer overflows.  FIXME */
#define PATH_MAX 4096
#endif

/* V8Js Version */
#define PHP_V8JS_VERSION "2.0.0"

/* Helper macros */
#define V8JS_GET_CLASS_NAME(var, obj) \
	v8::String::Utf8Value var(obj->GetConstructorName());

/* Options */
#define V8JS_FLAG_NONE			(1<<0)
#define V8JS_FLAG_FORCE_ARRAY	(1<<1)
#define V8JS_FLAG_PROPAGATE_PHP_EXCEPTIONS	(1<<2)


/* These are not defined by Zend */
#define ZEND_WAKEUP_FUNC_NAME    "__wakeup"
#define ZEND_SLEEP_FUNC_NAME     "__sleep"
#define ZEND_SET_STATE_FUNC_NAME "__set_state"


/* Convert zval into V8 value */
v8::Local<v8::Value> zval_to_v8js(zval *, v8::Isolate *);

/* Convert zend_long into V8 value */
v8::Local<v8::Value> zend_long_to_v8js(zend_long, v8::Isolate *);

/* Convert V8 value into zval */
int v8js_to_zval(v8::Local<v8::Value>, zval *, int, v8::Isolate *);

struct v8js_accessor_ctx
{
	zend_string *variable_name;
    v8::Isolate *isolate;
};

void v8js_accessor_ctx_dtor(v8js_accessor_ctx *);

/* Register accessors into passed object */
void v8js_register_accessors(std::vector<v8js_accessor_ctx*> *accessor_list, v8::Local<v8::FunctionTemplate>, zval *, v8::Isolate *);


/* Forward declarations */
struct v8js_timer_ctx;

/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(v8js)
  // Thread-local cache whether V8 has been initialized so far
  bool v8_initialized;

  /* Ini globals */
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

#define V8JSG(v) ZEND_MODULE_GLOBALS_ACCESSOR(v8js, v)

/*
 *  Process-wide globals
 *
 * The zend_v8js_globals structure declared above is created once per thread
 * (in a ZTS environment).  If a multi-threaded PHP process uses V8 there is
 * some stuff shared among all of these threads
 *
 *  - whether V8 has been initialized at all
 *  - the V8 backend platform
 *  - loaded extensions
 *  - V8 "command line" flags
 *
 * In a ZTS-enabled environment access to all of these variables must happen
 * while holding a mutex lock.
 */
struct _v8js_process_globals {
#ifdef ZTS
	bool v8_initialized;
	std::mutex lock;
#endif

	HashTable *extensions;

	/* V8 command line flags */
	char *v8_flags;

	/* Path to icudtl.dat file */
	char *icudtl_dat_path;

	v8::Platform *v8_platform;
};

extern struct _v8js_process_globals v8js_process_globals;

/* Register builtin methods into passed object */
void v8js_register_methods(v8::Local<v8::ObjectTemplate>, v8js_ctx *c);

#endif	/* PHP_V8JS_MACROS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
