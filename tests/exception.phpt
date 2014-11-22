--TEST--
Test V8::executeString() : V8JsScriptException
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
this_function_does_not_exist();
EOT;

$v8 = new V8Js();

try {
	$v8->executeString($JS, 'exception.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
object(V8JsScriptException)#%d (13) {
  ["message":protected]=>
  string(75) "exception.js:1: ReferenceError: this_function_does_not_exist is not defined"
  ["string":"Exception":private]=>
  string(0) ""
  ["code":protected]=>
  int(0)
  ["file":protected]=>
  string(%d) "%s"
  ["line":protected]=>
  int(10)
  ["trace":"Exception":private]=>
  array(1) {
    [0]=>
    array(6) {
      ["file"]=>
      string(%d) "%s"
      ["line"]=>
      int(10)
      ["function"]=>
      string(13) "executeString"
      ["class"]=>
      string(4) "V8Js"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(2) {
        [0]=>
        string(31) "this_function_does_not_exist();"
        [1]=>
        string(12) "exception.js"
      }
    }
  }
  ["previous":"Exception":private]=>
  NULL
  ["JsFileName":protected]=>
  string(12) "exception.js"
  ["JsLineNumber":protected]=>
  int(1)
  ["JsStartColumn":protected]=>
  int(0)
  ["JsEndColumn":protected]=>
  int(1)
  ["JsSourceLine":protected]=>
  string(31) "this_function_does_not_exist();"
  ["JsTrace":protected]=>
  string(83) "ReferenceError: this_function_does_not_exist is not defined
    at exception.js:1:1"
}
===EOF===
