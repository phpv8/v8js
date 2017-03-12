# add json extension, if needed (ie, for PHP >= 5.5)
ifneq (,$(realpath $(EXTENSION_DIR)/json.so))
PHP_TEST_SHARED_EXTENSIONS+=-d extension=$(EXTENSION_DIR)/json.so
endif

# add pthreads extension, if available
ifneq (,$(realpath $(EXTENSION_DIR)/pthreads.so))
PHP_TEST_SHARED_EXTENSIONS+=-d extension=$(EXTENSION_DIR)/pthreads.so
endif

# add dom extension, if available
ifneq (,$(realpath $(EXTENSION_DIR)/dom.so))
PHP_TEST_SHARED_EXTENSIONS+=-d extension=$(EXTENSION_DIR)/dom.so
endif
