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

extern "C" {
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
}

#include "v8js_class.h"
#include "v8js_exceptions.h"
#include "v8js_v8object_class.h"

ZEND_DECLARE_MODULE_GLOBALS(v8js)
struct _v8js_process_globals v8js_process_globals;

/* {{{ INI Settings */

static bool v8js_ini_string(char **field, const zend_string *new_value)/* {{{ */
{
	bool immutable = false;

#ifdef ZTS
	v8js_process_globals.lock.lock();

	if(v8js_process_globals.v8_initialized) {
		v8js_process_globals.lock.unlock();
		immutable = true;
	}

	v8js_process_globals.lock.unlock();
#else
	immutable = V8JSG(v8_initialized);
#endif

	if(immutable) {
		/* V8 already has been initialized -> cannot be changed anymore */
		return FAILURE;
	}

	if (new_value) {
		if (*field) {
			free(*field);
			*field = NULL;
		}

		if (!ZSTR_VAL(new_value)[0]) {
			return SUCCESS;
		}

		*field = zend_strndup(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
	}

	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateV8Flags) /* {{{ */
{
	return v8js_ini_string(&v8js_process_globals.v8_flags, new_value);
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateIcudatPath) /* {{{ */
{
	return v8js_ini_string(&v8js_process_globals.icudtl_dat_path, new_value);
}
/* }}} */

static bool v8js_ini_to_bool(const zend_string *new_value) /* {{{ */
{
	if (ZSTR_LEN(new_value) == 2 && strcasecmp("on", ZSTR_VAL(new_value)) == 0) {
		return true;
    } else if (ZSTR_LEN(new_value) == 3 && strcasecmp("yes", ZSTR_VAL(new_value)) == 0) {
		return true;
	} else if (ZSTR_LEN(new_value) == 4 && strcasecmp("true", ZSTR_VAL(new_value)) == 0) {
		return true;
	} else {
		return 0 != atoi(ZSTR_VAL(new_value));
	}
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateUseDate) /* {{{ */
{
	V8JSG(use_date) = v8js_ini_to_bool(new_value);
	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateUseArrayAccess) /* {{{ */
{
	V8JSG(use_array_access) = v8js_ini_to_bool(new_value);
	return SUCCESS;
}
/* }}} */

ZEND_INI_BEGIN() /* {{{ */
	ZEND_INI_ENTRY("v8js.flags", NULL, ZEND_INI_ALL, v8js_OnUpdateV8Flags)
	ZEND_INI_ENTRY("v8js.icudtl_dat_path", NULL, ZEND_INI_ALL, v8js_OnUpdateIcudatPath)
	ZEND_INI_ENTRY("v8js.use_date", "0", ZEND_INI_ALL, v8js_OnUpdateUseDate)
	ZEND_INI_ENTRY("v8js.use_array_access", "0", ZEND_INI_ALL, v8js_OnUpdateUseArrayAccess)
ZEND_INI_END()
/* }}} */

/* }}} INI */


#ifdef COMPILE_DL_V8JS
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif

ZEND_GET_MODULE(v8js)
#endif


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(v8js)
{
	PHP_MINIT(v8js_class)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(v8js_exceptions)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(v8js_v8object_class)(INIT_FUNC_ARGS_PASSTHRU);

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(v8js)
{
	UNREGISTER_INI_ENTRIES();

	bool v8_initialized;

#ifdef ZTS
	v8_initialized = v8js_process_globals.v8_initialized;
#else
	v8_initialized = V8JSG(v8_initialized);
#endif

	if(v8_initialized) {
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
		// @fixme call virtual destructor somehow
		//delete v8js_process_globals.v8_platform;
	}

	if (v8js_process_globals.v8_flags) {
		free(v8js_process_globals.v8_flags);
		v8js_process_globals.v8_flags = NULL;
	}

	if (v8js_process_globals.extensions) {
		zend_hash_destroy(v8js_process_globals.extensions);
		free(v8js_process_globals.extensions);
		v8js_process_globals.extensions = NULL;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(v8js)
{
	// If the timer thread is running then stop it
	if (V8JSG(timer_thread)) {
		V8JSG(timer_stop) = true;
		V8JSG(timer_thread)->join();
		V8JSG(timer_stop) = false;
		delete V8JSG(timer_thread);
		V8JSG(timer_thread) = NULL;
	}

	V8JSG(fatal_error_abort) = 0;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(v8js)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "V8 Javascript Engine", "enabled");
	php_info_print_table_header(2, "V8 Engine Compiled Version", PHP_V8_VERSION);
	php_info_print_table_header(2, "V8 Engine Linked Version", v8::V8::GetVersion());
	php_info_print_table_header(2, "Version", PHP_V8JS_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(v8js)
{
#if defined(COMPILE_DL_MYSQLI) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

	/*
	  If ZTS is disabled, the v8js_globals instance is declared right
	  in the BSS and hence automatically initialized by C++ compiler.
	  Most of the variables are just zeroed.

	  If ZTS is enabled however, v8js_globals just points to a freshly
	  allocated, uninitialized piece of memory, hence we need to
	  initialize all fields on our own.  Likewise on shutdown we have to
	  run the destructors manually.
	*/
#ifdef ZTS
	v8js_globals->v8_initialized = 0;

	v8js_globals->timer_thread = NULL;
	v8js_globals->timer_stop = false;
	new(&v8js_globals->timer_mutex) std::mutex;
	new(&v8js_globals->timer_stack) std::deque<v8js_timer_ctx *>;

	v8js_globals->fatal_error_abort = 0;
#endif
}
/* }}} */

/* {{{ PHP_GSHUTDOWN_FUNCTION
 */
static PHP_GSHUTDOWN_FUNCTION(v8js)
{
#ifdef ZTS
	v8js_globals->timer_stack.~deque();
	v8js_globals->timer_mutex.~mutex();
#endif
}
/* }}} */

/* {{{ v8js_functions[] */
static const zend_function_entry v8js_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ v8js_module_entry
 */
zend_module_entry v8js_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	"v8js",
	v8js_functions,
	PHP_MINIT(v8js),
	PHP_MSHUTDOWN(v8js),
	NULL,
	PHP_RSHUTDOWN(v8js),
	PHP_MINFO(v8js),
	PHP_V8JS_VERSION,
	PHP_MODULE_GLOBALS(v8js),
	PHP_GINIT(v8js),
	PHP_GSHUTDOWN(v8js),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
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
