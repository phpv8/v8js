--TEST--
Test V8::executeString() : correct temp context construction
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

class Failer {
    protected $_b = null;

    function call($a) {
        $this->_b = $a;
    } 

    function test() {
        print_r($this->_b);
    }
}

$v8->failer = new Failer();
$v8->executeString('PHP.failer.call({ foo: 23 });');
$v8->failer->test();
?>
===EOF===
--EXPECT--
V8Object Object
(
    [foo] => 23
)
===EOF===
