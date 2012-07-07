/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
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
  | Author: Patrick Reilly <preilly@php.net>                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
}

#include "php_v8js_macros.h"
#include <v8.h>

/* global.exit - terminate execution */
V8JS_METHOD(exit) /* {{{ */
{
	v8::V8::TerminateExecution();
	return v8::Undefined();
}
/* }}} */

/* global.sleep - sleep for passed seconds */
V8JS_METHOD(sleep) /* {{{ */
{
	php_sleep(args[0]->Int32Value());
	return v8::Undefined();
}
/* }}} */

/* global.print - php print() */
V8JS_METHOD(print) /* {{{ */
{
	int ret = 0;
	TSRMLS_FETCH();

	for (int i = 0; i < args.Length(); i++) {
		v8::String::Utf8Value str(args[i]);
		const char *cstr = ToCString(str);
		ret = PHPWRITE(cstr, strlen(cstr));
	}
	return V8JS_INT(ret);
}
/* }}} */

static void _php_v8js_dumper(v8::Local<v8::Value> var, int level TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value str(var->ToDetailString());
	const char *valstr = ToCString(str);
	size_t valstr_len = (valstr) ? strlen(valstr) : 0;

	if (level > 1) {
		php_printf("%*c", (level - 1) * 2, ' ');
	}

	if (var->IsString())
	{
		php_printf("string(%d) \"%s\"\n", valstr_len, valstr);
	}
	else if (var->IsBoolean())
	{
		php_printf("bool(%s)\n", valstr);
	}
	else if (var->IsInt32() || var->IsUint32())
	{
		php_printf("int(%s)\n", valstr);
	}
	else if (var->IsNumber())
	{
		php_printf("float(%s)\n", valstr);
	}
	else if (var->IsDate())
	{
		php_printf("Date(%s)\n", valstr);
	}
#if PHP_V8_API_VERSION >= 2003007
	else if (var->IsRegExp())
	{
		php_printf("RegExp(%s)\n", valstr);
	}
#endif
	else if (var->IsArray())
	{
		v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(var);
		uint32_t length = array->Length();

		php_printf("array(%d) {\n", length);

		for (unsigned i = 0; i < length; i++) {
			php_printf("%*c[%d] =>\n", level * 2, ' ', i);
			_php_v8js_dumper(array->Get(i), level + 1 TSRMLS_CC);
		}

		if (level > 1) {
			php_printf("%*c", (level - 1) * 2, ' ');
		}

		ZEND_PUTS("}\n");
	}
	else if (var->IsObject())
	{
		v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(var);
		V8JS_GET_CLASS_NAME(cname, object);

		if (var->IsFunction())
		{
			v8::String::Utf8Value csource(object->ToString());
			php_printf("object(%s)#%d {\n%*c%s\n", ToCString(cname), object->GetIdentityHash(), level * 2 + 2, ' ', ToCString(csource));
		}
		else
		{
			v8::Local<v8::Array> keys = object->GetPropertyNames();
			uint32_t length = keys->Length();

			php_printf("object(%s)#%d (%d) {\n", ToCString(cname), object->GetIdentityHash(), length);

			for (unsigned i = 0; i < length; i++) {
				v8::Local<v8::String> key = keys->Get(i)->ToString();
				v8::String::Utf8Value kname(key);
				php_printf("%*c[\"%s\"] =>\n", level * 2, ' ', ToCString(kname));
				_php_v8js_dumper(object->Get(key), level + 1 TSRMLS_CC);
			}
		}

		if (level > 1) {
			php_printf("%*c", (level - 1) * 2, ' ');
		}

		ZEND_PUTS("}\n");
	}
	else /* null, undefined, etc. */
	{
		php_printf("<%s>\n", valstr);
	}
}
/* }}} */

/* global.var_dump - Dump JS values */
V8JS_METHOD(var_dump) /* {{{ */
{
	int i;
	TSRMLS_FETCH();

	for (int i = 0; i < args.Length(); i++) {
		_php_v8js_dumper(args[i], 1 TSRMLS_CC);
	}
	
	return V8JS_NULL;
}
/* }}} */

void php_v8js_register_methods(v8::Handle<v8::ObjectTemplate> global) /* {{{ */
{
	global->Set(V8JS_SYM("exit"), v8::FunctionTemplate::New(V8JS_MN(exit)), v8::ReadOnly);
	global->Set(V8JS_SYM("sleep"), v8::FunctionTemplate::New(V8JS_MN(sleep)), v8::ReadOnly);
	global->Set(V8JS_SYM("print"), v8::FunctionTemplate::New(V8JS_MN(print)), v8::ReadOnly);
	global->Set(V8JS_SYM("var_dump"), v8::FunctionTemplate::New(V8JS_MN(var_dump)), v8::ReadOnly);
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
