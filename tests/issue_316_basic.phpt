--TEST--
Test V8::executeString() : Issue #316 endless property iteration
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$values = $v8->executeString('({test: "test"})');
foreach ($values as $value) {
    var_dump($value);
}

?>
===EOF===
--EXPECT--
string(4) "test"
===EOF===
