--TEST--
Test V8::executeString() : Handle die() gracefully
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();
$v8->foo = function() {
    echo "Hallo\n";
    die("Goodbye cruel world!\n");
};
$v8->executeString('PHP.foo();');
?>
===EOF===
--EXPECT--
Hallo
Goodbye cruel world!
