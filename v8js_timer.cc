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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}

#include "php_v8js_macros.h"
#include "v8js_v8.h"
#include "v8js_exceptions.h"

static void v8js_timer_interrupt_handler(v8::Isolate *isolate, void *data) { /* {{{ */
#ifdef ZTS
	TSRMLS_D = (void ***) data;
#endif

	if (!V8JSG(timer_stack).size()) {
		return;
	}

	v8::Locker locker(isolate);
	v8::HeapStatistics hs;
	isolate->GetHeapStatistics(&hs);

	V8JSG(timer_mutex).lock();

	for (std::deque< v8js_timer_ctx* >::iterator it = V8JSG(timer_stack).begin();
		 it != V8JSG(timer_stack).end(); it ++) {
		v8js_timer_ctx *timer_ctx = *it;
		v8js_ctx *c = timer_ctx->ctx;

		if(c->isolate != isolate || timer_ctx->killed) {
			continue;
		}

		if (timer_ctx->memory_limit > 0 && hs.used_heap_size() > timer_ctx->memory_limit) {
			timer_ctx->killed = true;
			v8js_terminate_execution(c TSRMLS_CC);
			c->memory_limit_hit = true;
		}
	}

	V8JSG(timer_mutex).unlock();
}
/* }}} */

void v8js_timer_thread(TSRMLS_D) /* {{{ */
{
	while (!V8JSG(timer_stop)) {

		V8JSG(timer_mutex).lock();
		if (V8JSG(timer_stack).size()) {
			v8js_timer_ctx *timer_ctx = V8JSG(timer_stack).front();
			v8js_ctx *c = timer_ctx->ctx;
			std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();

			if(timer_ctx->killed) {
				/* execution already terminated, nothing to check anymore,
				 * but wait for caller to pop this timer context. */
			}
			else if(timer_ctx->time_limit > 0 && now > timer_ctx->time_point) {
				timer_ctx->killed = true;
				v8js_terminate_execution(c TSRMLS_CC);
				c->time_limit_hit = true;
			}
			else if (timer_ctx->memory_limit > 0) {
				/* If a memory_limit is set, we need to interrupt execution
				 * and check heap size within the callback.  We must *not*
				 * directly call GetHeapStatistics here, since we don't have
				 * a v8::Locker on the isolate, but are expected to hold one,
				 * and cannot aquire it as v8 is executing the script ... */
				void *data = NULL;
#ifdef ZTS
				data = (void *) TSRMLS_C;
#endif
				c->isolate->RequestInterrupt(v8js_timer_interrupt_handler, data);
			}
		}
		V8JSG(timer_mutex).unlock();

		// Sleep for 10ms
#ifdef _WIN32
		concurrency::wait(10);
#else
		std::chrono::milliseconds duration(10);
		std::this_thread::sleep_for(duration);
#endif
	}
}
/* }}} */


void v8js_timer_push(long time_limit, long memory_limit, v8js_ctx *c TSRMLS_DC) /* {{{ */
{
	V8JSG(timer_mutex).lock();

	// Create context for this timer
	v8js_timer_ctx *timer_ctx = (v8js_timer_ctx *)emalloc(sizeof(v8js_timer_ctx));

	// Calculate the time point when the time limit is exceeded
	std::chrono::milliseconds duration(time_limit);
	std::chrono::time_point<std::chrono::high_resolution_clock> from = std::chrono::high_resolution_clock::now();

	// Push the timer context
	timer_ctx->time_limit = time_limit;
	timer_ctx->memory_limit = memory_limit;
	timer_ctx->time_point = from + duration;
	timer_ctx->ctx = c;
	timer_ctx->killed = false;
	V8JSG(timer_stack).push_front(timer_ctx);

	V8JSG(timer_mutex).unlock();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
