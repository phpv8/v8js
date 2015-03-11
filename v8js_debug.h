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

#ifndef V8JS_DEBUG_H
#define V8JS_DEBUG_H

#include <v8-debug.h>

extern PHP_METHOD(V8Js, __destruct);
extern PHP_METHOD(V8Js, startDebugAgent);

extern v8js_ctx *v8js_debug_context;
extern int v8js_debug_auto_break_mode;

#endif /* V8JS_DEBUG_H */

