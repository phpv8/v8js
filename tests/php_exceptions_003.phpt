--TEST--
Test V8::executeString() : PHP Exception handling (basic JS propagation)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
    function throwException() {
	throw new \Exception("Test-Exception");
    }
}

$v8 = new V8Js();
$v8->foo = new \Foo();

$JS = <<< EOT
try {
    PHP.foo.throwException();
    // the exception should abort further execution,
    // hence the print must not pop up
    print("after throwException\\n");
} catch(e) {
    print("JS caught exception!\\n");
    var_dump(e.getMessage());
}
EOT;

$v8->executeString($JS, 'php_exceptions_003', V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);

?>
===EOF===
--EXPECTF--
JS caught exception!
string(14) "Test-Exception"
===EOF===
