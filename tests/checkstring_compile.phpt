--TEST--
Test V8::executeString() : Script validator test using compileString
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
var_dump($v8->compileString('print("Hello World!");'));

try {
	var_dump($v8->compileString('print("Hello World!);'));
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECTF--
resource(%d) of type (V8Js script)
string(%d) "V8Js::compileString():1: SyntaxError: %s"
===EOF===
