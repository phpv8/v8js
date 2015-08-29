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

Fatal error: Uncaught Error: Call to undefined method Foo::bar() in %s%efatal_error_v8function.php:6
Stack trace:
#0 [internal function]: Foo->callback()
#1 %s%efatal_error_v8function.php(14): V8Function->V8Js::V8::Invoke()
#2 {main}
  thrown in %s%efatal_error_v8function.php on line 6
