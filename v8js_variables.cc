/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  +----------------------------------------------------------------------+
*/

/* $Id: v8js_variables.cc 306926 2010-12-31 14:15:54Z felipe $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
}

#include "php_v8js_macros.h"
#include <v8.h>

static v8::Handle<v8::Value> php_v8js_fetch_php_variable(v8::Local<v8::String> name, const v8::AccessorInfo &info) /* {{{ */
{
	v8::String::Utf8Value variable_name(info.Data()->ToString());
	const char *variable_name_string = ToCString(variable_name);
	uint variable_name_string_len = strlen(variable_name_string);
	zval **variable;

	TSRMLS_FETCH();

	zend_is_auto_global(variable_name_string, variable_name_string_len TSRMLS_CC);

	if (zend_hash_find(&EG(symbol_table), variable_name_string, variable_name_string_len + 1, (void **) &variable) == SUCCESS) {
		return zval_to_v8js(*variable TSRMLS_CC);
	}
	return v8::Undefined();
}
/* }}} */

void php_v8js_register_accessors(v8::Local<v8::ObjectTemplate> php_obj, zval *array TSRMLS_DC) /* {{{ */
{
	char *property_name;
	uint property_name_len;
	ulong index;
	zval **item;

	for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(array));
		zend_hash_get_current_data(Z_ARRVAL_P(array), (void **) &item) != FAILURE;
		zend_hash_move_forward(Z_ARRVAL_P(array))
	) {
		switch (Z_TYPE_PP(item))
		{
			/*
			case IS_OBJECT:
			case IS_ARRAY:
			*/
			case IS_STRING:
				break;

			default:
				continue; /* Ignore invalid values */
		}

		if (zend_hash_get_current_key_ex(Z_ARRVAL_P(array), &property_name, &property_name_len, &index, 0, NULL) != HASH_KEY_IS_STRING) {
			continue; /* Ignore invalid property names */
		}

		/* Set the variable fetch callback for given symbol on named property */
		php_obj->SetAccessor(V8JS_STRL(property_name, property_name_len - 1), php_v8js_fetch_php_variable, NULL, V8JS_STRL(Z_STRVAL_PP(item), Z_STRLEN_PP(item)), v8::PROHIBITS_OVERWRITING, v8::ReadOnly);
	}
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
