--TEST--
Test V8::executeString() : Check passing array from JS to PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$v8->test = function ($arr) { var_dump($arr); };
try {
	$v8->executeString('PHP.test([0, 1, 2]);');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
	$v8->executeString('PHP.test({ "0" : "0", "1" : "1", "2" : "2" });');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
	$v8->executeString('PHP.test({ "0" : "foo", "1" : "bar", "2" : "baz" });');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
	$v8->executeString('PHP.test({ "foo" : "0", "bar" : "1", "baz" : "2" });');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try { 
  // includes gap
	$v8->executeString('PHP.test({ "0" : "0", "2" : "2", "3" : "3" });');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
  // mixed key types
	$v8->executeString('PHP.test({ "0" : "0", "bar" : "1", "2" : "2" });');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

?>
===EOF===
--EXPECT--
array(3) {
  [0]=>
  int(0)
  [1]=>
  int(1)
  [2]=>
  int(2)
}
object(V8Object)#3 (3) {
  ["0"]=>
  string(1) "0"
  ["1"]=>
  string(1) "1"
  ["2"]=>
  string(1) "2"
}
object(V8Object)#3 (3) {
  ["0"]=>
  string(3) "foo"
  ["1"]=>
  string(3) "bar"
  ["2"]=>
  string(3) "baz"
}
object(V8Object)#3 (3) {
  ["foo"]=>
  string(1) "0"
  ["bar"]=>
  string(1) "1"
  ["baz"]=>
  string(1) "2"
}
object(V8Object)#3 (3) {
  ["0"]=>
  string(1) "0"
  ["2"]=>
  string(1) "2"
  ["3"]=>
  string(1) "3"
}
object(V8Object)#3 (3) {
  ["0"]=>
  string(1) "0"
  ["2"]=>
  string(1) "2"
  ["bar"]=>
  string(1) "1"
}
===EOF===
