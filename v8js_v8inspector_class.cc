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

#include <iostream>

#include <v8-inspector.h>


extern "C" {
// #include "ext/date/php_date.h"
// #include "ext/standard/php_string.h"
// #include "zend_interfaces.h"
// #include "zend_closures.h"
// #include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}


/* {{{ Class Entries */
zend_class_entry *php_ce_v8inspector;
/* }}} */

/* {{{ Object Handlers */
static zend_object_handlers v8js_v8inspector_handlers;
/* }}} */



#define kInspectorClientIndex 2

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
	public:
		explicit InspectorFrontend(v8::Local<v8::Context> context) {
			isolate_ = context->GetIsolate();
			// context_.Reset(isolate_, context);
		}
		~InspectorFrontend() override = default;

	private:
		void sendResponse(
				int callId,
				std::unique_ptr<v8_inspector::StringBuffer> message) override {
			Send(message->string());
		}
		void sendNotification(
				std::unique_ptr<v8_inspector::StringBuffer> message) override {
			Send(message->string());
		}
		void flushProtocolNotifications() override {}


		void Send(const v8_inspector::StringView& string) {
			v8::Isolate::AllowJavascriptExecutionScope allow_script(isolate_);
			v8::HandleScope handle_scope(isolate_);
			int length = static_cast<int>(string.length());
			// DCHECK_LT(length, v8::String::kMaxLength);
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
			// RETVAL_STRINGL(cstr, str.length());
			std::cout << cstr << std::endl;
		}

		v8::Isolate* isolate_;
		// Global<Context> context_;
};


class InspectorClient : public v8_inspector::V8InspectorClient {
	public:
		InspectorClient(v8::Local<v8::Context> context) {
			isolate_ = context->GetIsolate();
			channel_.reset(new InspectorFrontend(context));
			inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
			session_ =
				inspector_->connect(1, channel_.get(), v8_inspector::StringView());
			context->SetAlignedPointerInEmbedderData(kInspectorClientIndex, this);
			inspector_->contextCreated(v8_inspector::V8ContextInfo(
						context, kContextGroupId, v8_inspector::StringView()));

			/* Local<Value> function =
				FunctionTemplate::New(isolate_, SendInspectorMessage)
				->GetFunction(context)
				.ToLocalChecked();
			Local<String> function_name = String::NewFromUtf8Literal(
					isolate_, "send", NewStringType::kInternalized);
			CHECK(context->Global()->Set(context, function_name, function).FromJust()); */

			context_.Reset(isolate_, context);
		}

		/* void runMessageLoopOnPause(int contextGroupId) override {
			v8::Isolate::AllowJavascriptExecutionScope allow_script(isolate_);
			v8::HandleScope handle_scope(isolate_);
			Local<String> callback_name = v8::String::NewFromUtf8Literal(
					isolate_, "handleInspectorMessage", NewStringType::kInternalized);
			Local<Context> context = context_.Get(isolate_);
			Local<Value> callback =
				context->Global()->Get(context, callback_name).ToLocalChecked();
			if (!callback->IsFunction()) return;

			v8::TryCatch try_catch(isolate_);
			try_catch.SetVerbose(true);
			is_paused = true;

			while (is_paused) {
				USE(Local<Function>::Cast(callback)->Call(context, Undefined(isolate_), 0,
							{}));
				if (try_catch.HasCaught()) {
					is_paused = false;
				}
			}
		}

		void quitMessageLoopOnPause() override { is_paused = false; } */

	private:
		static v8_inspector::V8InspectorSession* GetSession(v8::Local<v8::Context> context) {
			InspectorClient* inspector_client = static_cast<InspectorClient*>(
					context->GetAlignedPointerFromEmbedderData(kInspectorClientIndex));
			return inspector_client->session_.get();
		}

		/* Local<Context> ensureDefaultContextInGroup(int group_id) override {
			DCHECK(isolate_);
			DCHECK_EQ(kContextGroupId, group_id);
			return context_.Get(isolate_);
		} */

		/* static void SendInspectorMessage(
				const v8::FunctionCallbackInfo<v8::Value>& args) {
			Isolate* isolate = args.GetIsolate();
			v8::HandleScope handle_scope(isolate);
			Local<Context> context = isolate->GetCurrentContext();
			args.GetReturnValue().Set(Undefined(isolate));
			Local<String> message = args[0]->ToString(context).ToLocalChecked();
			v8_inspector::V8InspectorSession* session =
				InspectorClient::GetSession(context);
			int length = message->Length();
			std::unique_ptr<uint16_t[]> buffer(new uint16_t[length]);
			message->Write(isolate, buffer.get(), 0, length);
			v8_inspector::StringView message_view(buffer.get(), length);
			{
				v8::SealHandleScope seal_handle_scope(isolate);
				session->dispatchProtocolMessage(message_view);
			}
			args.GetReturnValue().Set(True(isolate));
		} */

		static const int kContextGroupId = 1;

		std::unique_ptr<v8_inspector::V8Inspector> inspector_;
		std::unique_ptr<v8_inspector::V8InspectorSession> session_;
		std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
		// bool is_paused = false;
		v8::Global<v8::Context> context_;
		v8::Isolate* isolate_;
};



static void v8js_v8inspector_free_storage(zend_object *object) /* {{{ */
{
	v8js_v8inspector *c = v8js_v8inspector_fetch_object(object);

	delete c->client;

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
	// new(&c->v8obj) v8::Persistent<v8::Value>();

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

void v8js_v8inspector_create(zval *res, v8js_ctx *ctx) /* {{{ */
{
    object_init_ex(res, php_ce_v8inspector);
	v8js_v8inspector *inspector = Z_V8JS_V8INSPECTOR_OBJ_P(res);

	V8JS_CTX_PROLOGUE(ctx);
	inspector->client = new InspectorClient(v8_context);

}
/* }}} */

static const zend_function_entry v8js_v8inspector_methods[] = { /* {{{ */
	PHP_ME(V8Inspector,	__construct,			NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(V8Inspector,	__sleep,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8Inspector,	__wakeup,				NULL,				ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
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
