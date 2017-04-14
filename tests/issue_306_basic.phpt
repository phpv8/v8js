--TEST--
Test V8::executeString() : Issue #306 V8 crashing on toLocaleString()
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8 = new V8Js;
$expr = 'new Date("10/11/2009").toLocaleString("en-us", { month: "long" });';
var_dump( $v8->executeString($expr, null, V8Js::FLAG_FORCE_ARRAY) );

?>
===EOF===
--EXPECT--
string(7) "October"
===EOF===
