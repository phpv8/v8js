--TEST--
Test V8::executeString() : Pass JS date to PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
	
ini_set('v8js.use_date', 1);
$a = new V8Js();
$a->var = new \DateTime("Wed, 19 Mar 2014 14:37:11 +0000");
$a->executeString('print(PHP.var.toGMTString()); print("\n");');
ini_set('v8js.use_date', 0);

$a = new V8Js();
$a->var = new \DateTime("Wed, 19 Mar 2014 14:37:11 +0000");
$a->executeString('print(PHP.var.toString()); print("\n");');

?>
===EOF===
--EXPECT--
Wed, 19 Mar 2014 14:37:11 GMT
[object DateTime]
===EOF===
