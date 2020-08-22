/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2020 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"
#include "v8js_exceptions.h"
#include "v8js_v8inspector_class.h"

#include <v8-inspector.h>

extern "C" {
#include "zend_exceptions.h"
}


/* {{{ Class Entries */
zend_class_entry *php_ce_v8inspector;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_v8inspector_handlers;
/* }}} */



class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
	public:
		explicit InspectorFrontend(v8::Local<v8::Context> context, zval *response_handler, zval *notification_handler) {
			isolate_ = context->GetIsolate();
			response_handler_ = response_handler;
			notification_handler_ = notification_handler;
		}
		~InspectorFrontend() override = default;

	private:
		void sendResponse(
				int callId,
				std::unique_ptr<v8_inspector::StringBuffer> message) override {
			invokeHandler(message->string(), response_handler_);
		}
		void sendNotification(
				std::unique_ptr<v8_inspector::StringBuffer> message) override {
			invokeHandler(message->string(), notification_handler_);
		}
		void flushProtocolNotifications() override {}


		void invokeHandler(const v8_inspector::StringView& string, zval *handler) {
			if (Z_TYPE_P(handler) == IS_NULL) {
				return;
			}

			v8::HandleScope handle_scope(isolate_);
			int length = static_cast<int>(string.length());
			v8::Local<v8::String> message =
				(string.is8Bit()
				 ? v8::String::NewFromOneByte(
					 isolate_,
					 reinterpret_cast<const uint8_t*>(string.characters8()),
					 v8::NewStringType::kNormal, length)
				 : v8::String::NewFromTwoByte(
					 isolate_,
					 reinterpret_cast<const uint16_t*>(string.characters16()),
					 v8::NewStringType::kNormal, length))
				.ToLocalChecked();

			v8::String::Utf8Value str(isolate_, message);
			const char *cstr = ToCString(str);

			zval handler_result;
			zval params[1];
			ZVAL_STRINGL(&params[0], cstr, str.length());
			call_user_function_ex(EG(function_table), NULL, handler, &handler_result, 1, params, 0, NULL);
		}

		v8::Isolate* isolate_;
		zval *response_handler_;
		zval *notification_handler_;
};


class InspectorClient : public v8_inspector::V8InspectorClient {
	public:
		InspectorClient(v8::Local<v8::Context> context, zval *response_handler, zval *notification_handler) {
			isolate_ = context->GetIsolate();
			channel_.reset(new InspectorFrontend(context, response_handler, notification_handler));
			inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
			session_ = inspector_->connect(1, channel_.get(), v8_inspector::StringView());
			inspector_->contextCreated(v8_inspector::V8ContextInfo(
						context, kContextGroupId, v8_inspector::StringView()));
			context_.Reset(isolate_, context);
		}

		void send(const zend_string *message) {
			v8::Locker locker(isolate_);
			v8::HandleScope handle_scope(isolate_);
			v8_inspector::StringView message_view((const uint8_t*) ZSTR_VAL(message), ZSTR_LEN(message));
			{
				v8::SealHandleScope seal_handle_scope(isolate_);
				session_->dispatchProtocolMessage(message_view);
			}
		}

	private:
		static const int kContextGroupId = 1;

		std::unique_ptr<v8_inspector::V8Inspector> inspector_;
		std::unique_ptr<v8_inspector::V8InspectorSession> session_;
		std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
		v8::Global<v8::Context> context_;
		v8::Isolate* isolate_;
};



static void v8js_v8inspector_free_storage(zend_object *object) /* {{{ */
{
	v8js_v8inspector *c = v8js_v8inspector_fetch_object(object);

	delete c->client;
	zval_ptr_dtor(&c->response_handler);
	zval_ptr_dtor(&c->notification_handler);

	zend_object_std_dtor(&c->std);
}
/* }}} */

static zend_object *v8js_v8inspector_new(zend_class_entry *ce) /* {{{ */
{
	v8js_v8inspector *c;
	c = (v8js_v8inspector *) ecalloc(1, sizeof(v8js_v8inspector) + zend_object_properties_size(ce));

	zend_object_std_init(&c->std, ce);
	object_properties_init(&c->std, ce);

	c->std.handlers = &v8js_v8inspector_handlers;

	ZVAL_NULL(&c->response_handler);
	ZVAL_NULL(&c->notification_handler);

	return &c->std;
}
/* }}} */


/* {{{ proto V8Inspector::__construct()
 */
PHP_METHOD(V8Inspector,__construct)
{
	zend_throw_exception(php_ce_v8js_exception,
		"Can't directly construct V8Inspector objects!", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Inspector::__sleep()
 */
PHP_METHOD(V8Inspector, __sleep)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Inspector instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto V8Inspector::__wakeup()
 */
PHP_METHOD(V8Inspector, __wakeup)
{
	zend_throw_exception(php_ce_v8js_exception,
		"You cannot serialize or unserialize V8Inspector instances", 0);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto void V8Inspector::send(string message)
 */
static PHP_METHOD(V8Inspector, send)
{
	zend_string *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &message) == FAILURE) {
		return;
	}

	v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(getThis());
	inspector->client->send(message);

}
/* }}} */

/* {{{ proto void V8Inspector::setResponseHandler(callable)
 */
static PHP_METHOD(V8Inspector, setResponseHandler)
{
	v8js_ctx *c;
	zval *callable;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &callable) == FAILURE) {
		return;
	}

	v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(getThis());
	ZVAL_COPY(&inspector->response_handler, callable);
}
/* }}} */

/* {{{ proto void V8Inspector::setNotificationHandler(callable)
 */
static PHP_METHOD(V8Inspector, setNotificationHandler)
{
	v8js_ctx *c;
	zval *callable;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &callable) == FAILURE) {
		return;
	}

	v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(getThis());
	ZVAL_COPY(&inspector->notification_handler, callable);
}
/* }}} */


void v8js_v8inspector_create(zval *res, v8js_ctx *ctx) /* {{{ */
{
    object_init_ex(res, php_ce_v8inspector);
	v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(res);

	V8JS_CTX_PROLOGUE(ctx);
	inspector->client = new InspectorClient(v8_context,
			&inspector->response_handler, &inspector->notification_handler);

}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8inspector_send, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8inspector_set_handler, 0, 0, 1)
	ZEND_ARG_INFO(0, callable)
ZEND_END_ARG_INFO()

static const zend_function_entry v8js_v8inspector_methods[] = { /* {{{ */
	PHP_ME(V8Inspector,	__construct,			NULL,								ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Inspector,	__sleep,				NULL,								ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Inspector,	__wakeup,				NULL,								ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Inspector,	send,					arginfo_v8inspector_send, 			ZEND_ACC_PUBLIC)
	PHP_ME(V8Inspector, setResponseHandler,		arginfo_v8inspector_set_handler,	ZEND_ACC_PUBLIC)
	PHP_ME(V8Inspector, setNotificationHandler,	arginfo_v8inspector_set_handler,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(v8js_v8inspector_class) /* {{{ */
{
	zend_class_entry ce;

	/* V8Inspector Class */
	INIT_CLASS_ENTRY(ce, "V8Inspector", v8js_v8inspector_methods);
	php_ce_v8inspector = zend_register_internal_class(&ce);
	php_ce_v8inspector->ce_flags |= ZEND_ACC_FINAL;
	php_ce_v8inspector->create_object = v8js_v8inspector_new;

	/* V8Inspector handlers */
	memcpy(&v8js_v8inspector_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	v8js_v8inspector_handlers.clone_obj = NULL;
	v8js_v8inspector_handlers.cast_object = NULL;
	v8js_v8inspector_handlers.offset = XtOffsetOf(struct v8js_v8inspector, std);
	v8js_v8inspector_handlers.free_obj = v8js_v8inspector_free_storage;

	return SUCCESS;
} /* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
