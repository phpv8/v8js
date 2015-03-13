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

#define V8JS_DEBUG 0
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"

extern "C" {
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_smart_str.h"
}

#include "v8js_class.h"
#include "v8js_exceptions.h"
#include "v8js_v8object_class.h"

ZEND_DECLARE_MODULE_GLOBALS(v8js)

/* {{{ INI Settings */

static ZEND_INI_MH(v8js_OnUpdateV8Flags) /* {{{ */
{
	if (new_value) {
		if (V8JSG(v8_flags)) {
			free(V8JSG(v8_flags));
			V8JSG(v8_flags) = NULL;
		}
		if (!new_value[0]) {
			return FAILURE;
		}
		V8JSG(v8_flags) = zend_strndup(new_value, new_value_length);
	}

	return SUCCESS;
}

static ZEND_INI_MH(v8js_OnUpdateUseDate) /* {{{ */
{
	bool value;
	if (new_value_length==2 && strcasecmp("on", new_value)==0) {
		value = (bool) 1;
    } else if (new_value_length==3 && strcasecmp("yes", new_value)==0) {
		value = (bool) 1;
	} else if (new_value_length==4 && strcasecmp("true", new_value)==0) {
		value = (bool) 1;
	} else {
		value = (bool) atoi(new_value);
	}
	V8JSG(use_date) = value;
	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(v8js_OnUpdateUseArrayAccess) /* {{{ */
{
	bool value;
	if (new_value_length==2 && strcasecmp("on", new_value)==0) {
		value = (bool) 1;
    } else if (new_value_length==3 && strcasecmp("yes", new_value)==0) {
		value = (bool) 1;
	} else if (new_value_length==4 && strcasecmp("true", new_value)==0) {
		value = (bool) 1;
	} else {
		value = (bool) atoi(new_value);
	}
	V8JSG(use_array_access) = value;
	return SUCCESS;
}
/* }}} */

ZEND_INI_BEGIN() /* {{{ */
	ZEND_INI_ENTRY("v8js.flags", NULL, ZEND_INI_ALL, v8js_OnUpdateV8Flags)
	ZEND_INI_ENTRY("v8js.use_date", "0", ZEND_INI_ALL, v8js_OnUpdateUseDate)
	ZEND_INI_ENTRY("v8js.use_array_access", "0", ZEND_INI_ALL, v8js_OnUpdateUseArrayAccess)
ZEND_INI_END()
/* }}} */

/* }}} INI */


#ifdef COMPILE_DL_V8JS
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
	}

#if V8JS_DEBUG
	v8::HeapStatistics stats;
	v8::V8::GetHeapStatistics(&stats);
	float used = stats.used_heap_size() / 1024.0 / 1024.0;
	float total = stats.total_heap_size() / 1024.0 / 1024.0;

	fprintf(stderr, "### RSHUTDOWN ###\n");
	fprintf(stderr, "############ Heap Used/Total %.2f/%.2f MB ############\n", used, total);
	fflush(stderr);
#endif

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
	v8js_globals->extensions = NULL;
	v8js_globals->v8_initialized = 0;
	v8js_globals->v8_flags = NULL;

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
	if (v8js_globals->extensions) {
		zend_hash_destroy(v8js_globals->extensions);
		free(v8js_globals->extensions);
		v8js_globals->extensions = NULL;
	}

	if (v8js_globals->v8_flags) {
		free(v8js_globals->v8_flags);
		v8js_globals->v8_flags = NULL;
	}

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
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
