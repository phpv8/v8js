--TEST--
Test V8::executeString() : Extra properties on derived class
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class V8Wrapper extends V8Js {
    protected $testing;

    public function __construct() {
        parent::__construct();
        $this->testing = 23;
        $this->extraTesting = 42;
    }
}

$v8 = new V8Wrapper();
$v8->executeString('print(PHP.testing + "\n");');
$v8->executeString('print(PHP.extraTesting + "\n");');
?>
===EOF===
--EXPECT--
undefined
42
===EOF===
