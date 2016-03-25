--TEST--
Test V8::executeString() : Object passing JS > PHP > JS
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->theApiCall = function() use ($v8) {
	return $v8->executeString('({ foo: 23 })');
};

$v8->executeString('var_dump(PHP.theApiCall().constructor.name);');

?>
===EOF===
--EXPECT--
string(6) "Object"
===EOF===
