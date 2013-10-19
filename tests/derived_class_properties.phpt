--TEST--
Test V8::executeString() : Properties on derived class
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class V8Wrapper extends V8Js {
    public $testing;

    public function __construct() {
        parent::__construct();
        $this->testing = 23;
    }
}

$v8 = new V8Wrapper();
$v8->executeString('print("foo\n");');
?>
===EOF===
--EXPECT--
foo
===EOF===
