--TEST--
Test V8::executeString() : Test PHP object reusage
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

// Reusing should work..

$v8 = new V8Js();
$foo = array('foo' => 'Destructor did not mess anything!');
$v8->foobar = $foo;

$v8->executeString("print(PHP.foobar['foo'] + '\\n');", "dtor.js");

?>
===EOF===
--EXPECT--
Destructor did not mess anything!
===EOF===
