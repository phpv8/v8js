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

#ifndef V8JS_CLASS_H
#define V8JS_CLASS_H


/* Abbreviate long type names */
typedef v8::Persistent<v8::FunctionTemplate, v8::CopyablePersistentTraits<v8::FunctionTemplate> > v8js_tmpl_t;
typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> > v8js_persistent_obj_t;

/* Forward declarations */
struct v8js_v8object;
struct v8js_accessor_ctx;
struct _v8js_script;

struct cmp_str {
    bool operator()(char const *a, char const *b) const {
        return strcmp(a, b) < 0;
    }
};

/* {{{ Context container */
struct v8js_ctx {
  v8::Persistent<v8::String> object_name;
  v8::Persistent<v8::Context> context;
  zend_bool report_uncaught;
  zval pending_exception;
  int in_execution;
  v8::Isolate *isolate;

  long flags;

  long time_limit;
  bool time_limit_hit;
  long memory_limit;
  bool memory_limit_hit;

  v8js_tmpl_t global_template;
  v8js_tmpl_t array_tmpl;

  zval module_normaliser;
  zval module_loader;

  std::vector<char *> modules_stack;
  std::vector<char *> modules_base;
  std::map<char *, v8js_persistent_obj_t, cmp_str> modules_loaded;
  std::map<const zend_string *,v8js_tmpl_t> template_cache;

  std::map<zend_object *, v8js_persistent_obj_t> weak_objects;
  std::map<v8js_tmpl_t *, v8js_persistent_obj_t> weak_closures;
  std::map<v8js_tmpl_t *, v8js_tmpl_t> call_impls;
  std::map<zend_function *, v8js_tmpl_t> method_tmpls;

  std::list<v8js_v8object *> v8js_v8objects;

  std::vector<v8js_accessor_ctx *> accessor_list;
  std::vector<struct _v8js_script *> script_objects;
  char *tz;
#ifdef ZTS
  void ***zts_ctx;
#endif

  zend_object std;
};
/* }}} */

#ifdef ZTS
# define V8JS_TSRMLS_FETCH() TSRMLS_FETCH_FROM_CTX(((v8js_ctx *) isolate->GetData(0))->zts_ctx);
#else
# define V8JS_TSRMLS_FETCH()
#endif

static inline struct v8js_ctx *v8js_ctx_fetch_object(zend_object *obj) {
	return (struct v8js_ctx *)((char *)obj - XtOffsetOf(struct v8js_ctx, std));
}

#define Z_V8JS_CTX_OBJ_P(zv) v8js_ctx_fetch_object(Z_OBJ_P(zv));


PHP_MINIT_FUNCTION(v8js_class);

#endif /* V8JS_CLASS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
