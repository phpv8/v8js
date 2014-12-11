--TEST--
Test V8Js : class inheritance
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

var_dump(class_exists('V8JsException'));
var_dump(class_exists('V8JsScriptException'));
var_dump(class_exists('V8JsTimeLimitException'));
var_dump(class_exists('V8JsMemoryLimitException'));

var_dump(is_subclass_of(new V8JsException(), 'RuntimeException'));

var_dump(is_subclass_of(new V8JsScriptException(), 'V8JsException'));
var_dump(is_subclass_of(new V8JsTimeLimitException(), 'V8JsException'));
var_dump(is_subclass_of(new V8JsMemoryLimitException(), 'V8JsException'));

?>
===EOF===
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
===EOF===
