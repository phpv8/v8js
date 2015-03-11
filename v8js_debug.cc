/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@brokenpipe.de>                          |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
}

#include "php_v8js_macros.h"
#include "v8js_debug.h"

#ifdef ENABLE_DEBUGGER_SUPPORT

v8js_ctx *v8js_debug_context;
int v8js_debug_auto_break_mode;


static void DispatchDebugMessages() { /* {{{ */
	if(v8js_debug_context == NULL) {
		return;
	}

	v8::Isolate* isolate = v8js_debug_context->isolate;
	v8::Isolate::Scope isolate_scope(isolate);

	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context =
		v8::Local<v8::Context>::New(isolate, v8js_debug_context->context);
	v8::Context::Scope scope(context);

	v8::Debug::ProcessDebugMessages();
}
/* }}} */

/* {{{ proto void V8Js::__destruct()
   __destruct for V8Js */
PHP_METHOD(V8Js, __destruct)
{
	v8js_ctx *c = (v8js_ctx *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if(!c->isolate) {
		/* c->isolate is initialized by __construct, which wasn't called if this
		 * instance was deserialized (which we already caught in __wakeup). */
		return;
	}

	V8JS_CTX_PROLOGUE(c);
	if(v8js_debug_context == c) {
		v8::Debug::DisableAgent();
		v8js_debug_context = NULL;
	}
}
/* }}} */

/* {{{ proto bool V8Js::startDebugAgent(string agent_name[, int port[, int auto_break]])
 */
PHP_METHOD(V8Js, startDebugAgent)
{
	char *str = NULL;
	int str_len = 0;
	long port = 0, auto_break = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sll", &str, &str_len, &port, &auto_break) == FAILURE) {
		return;
	}

	if(!port) {
		port = 9222;
	}

	V8JS_BEGIN_CTX(c, getThis());

	if(v8js_debug_context == c) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Debug agent already started for this V8Js instance");
		RETURN_BOOL(0);
	}

	if(v8js_debug_context != NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Debug agent already started for a different V8Js instance");
		RETURN_BOOL(0);
	}

	v8js_debug_context = c;
	v8js_debug_auto_break_mode = auto_break;

	v8::Debug::SetDebugMessageDispatchHandler(DispatchDebugMessages, true);
	v8::Debug::EnableAgent(str_len ? str : "V8Js", port, auto_break > 0);

	if(auto_break) {
		/* v8::Debug::EnableAgent doesn't really do what we want it to do,
		   since it only breaks processing on the default isolate.
		   Hence just trigger another DebugBreak, no for our main isolate. */
		v8::Debug::DebugBreak(c->isolate);
	}

	RETURN_BOOL(1);
}
/* }}} */


#endif  /* ENABLE_DEBUGGER_SUPPORT */
