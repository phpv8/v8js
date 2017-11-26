--TEST--
Test V8Js::executeString : Global scope links global object
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var_dump(typeof global);
var_dump(global.var_dump === var_dump);

// also this is equal to global scope, at least in global execution context
// (i.e. off modules)
var_dump(this === global);
EOT;

$v8 = new V8Js();
$v8->executeString($JS);
?>
===EOF===
--EXPECT--
string(6) "object"
bool(true)
bool(true)
===EOF===
