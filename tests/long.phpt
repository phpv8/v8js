--TEST--
Test V8::executeString() : Check long integer handling from PHP to JS
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$v8->long = pow(2, 31);
try {
	$v8->executeString('print(PHP.long); print("\n");');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

$v8->long = pow(2, 31) + 1;
try {
	$v8->executeString('print(PHP.long); print("\n");');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

$v8->long = -pow(2, 31);
try {
	$v8->executeString('print(PHP.long); print("\n");');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

$v8->long = -pow(2, 31) - 1;
try {
	$v8->executeString('print(PHP.long); print("\n");');
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

?>
===EOF===
--EXPECT--
2147483648
2147483649
-2147483648
-2147483649
===EOF===
