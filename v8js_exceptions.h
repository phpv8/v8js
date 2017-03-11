/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifndef V8JS_EXCEPTIONS_H
#define V8JS_EXCEPTIONS_H

extern zend_class_entry *php_ce_v8js_exception;
extern zend_class_entry *php_ce_v8js_script_exception;
extern zend_class_entry *php_ce_v8js_time_limit_exception;
extern zend_class_entry *php_ce_v8js_memory_limit_exception;

void v8js_create_script_exception(zval *return_value, v8::Isolate *isolate, v8::TryCatch *try_catch);
void v8js_throw_script_exception(v8::Isolate *isolate, v8::TryCatch *try_catch);

PHP_MINIT_FUNCTION(v8js_exceptions);

#endif /* V8JS_EXCEPTIONS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
