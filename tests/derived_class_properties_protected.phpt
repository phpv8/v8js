--TEST--
Test V8::executeString() : Protected and private properties on derived class
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class V8Wrapper extends V8Js {
    protected $testing;
    private $privTesting;

    public function __construct() {
        parent::__construct();
        $this->testing = 23;
        $this->privTesting = 42;
    }
}

$v8 = new V8Wrapper();
$v8->executeString('print(PHP.testing + "\n");');
$v8->executeString('print(PHP.privTesting + "\n");');
?>
===EOF===
--EXPECT--
undefined
undefined
===EOF===
