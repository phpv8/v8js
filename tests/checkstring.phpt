--TEST--
Test V8::executeString() : Script validator test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
var_dump($v8->checkString('print("Hello World!");'));

try {
	var_dump($v8->checkString('print("Hello World!);'));
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECTF--
Deprecated: Function V8Js::checkString() is deprecated in %s on line %d
bool(true)

Deprecated: Function V8Js::checkString() is deprecated in %s on line %d
string(%d) "V8Js::checkString():1: SyntaxError: %s"
===EOF===
