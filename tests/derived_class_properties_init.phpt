--TEST--
Test V8::executeString() : Initialized properties on derived class
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class V8Wrapper extends V8Js {
    public $testing = 23;
    protected $protectedVar = 42;
    private $privateVar = 42;
}

$v8 = new V8Wrapper();
echo($v8->testing."\n");
$v8->executeString('print(PHP.testing + "\n");');
$v8->executeString('print(PHP.protectedVar + "\n");');
$v8->executeString('print(PHP.privateVar + "\n");');
?>
===EOF===
--EXPECT--
23
23
undefined
undefined
===EOF===
