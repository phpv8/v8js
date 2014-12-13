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

#ifndef V8JS_TIMER_H
#define V8JS_TIMER_H

// Timer context
struct v8js_timer_ctx
{
  long time_limit;
  long memory_limit;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
  v8js_ctx *ctx;
  bool killed;
};

void v8js_timer_thread(TSRMLS_D);
void v8js_timer_push(long time_limit, long memory_limit, v8js_ctx *c TSRMLS_DC);

#endif /* V8JS_TIMER_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
