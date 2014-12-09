--TEST--
Test V8Function() : Handle fatal errors gracefully
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
    function callback() {
        echo "goodbye cruel world!\n";
        $this->bar(); // trigger fatal error
    }
}

$sandbox = new V8Js();
$sandbox->foo = new Foo();

$cb = $sandbox->executeString('(function() { PHP.foo.callback(); });');
$cb();

?>
===EOF===
--EXPECTF--
goodbye cruel world!

Fatal error: Call to undefined method Foo::bar() in %s
