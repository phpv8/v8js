--TEST--
Test V8::executeString() : Simple test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$a = new V8Js();
$a->func = function ($arg) { echo "Hello {$arg}, I'm Closure!\n"; };

try {
  $a->executeString('print(PHP.func + "\n"); PHP.func("foobar");', "closure_test.js");
} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
?>
===EOF===
--EXPECT--
[object Closure]
Hello foobar, I'm Closure!
===EOF===
