--TEST--
Test V8::executeString() : PHP Exception handling (basic)
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
    // JS should not catch in default mode
    print("JS caught exception");
}
EOT;

try {
    $v8->executeString($JS);
} catch (Exception $e) {
    var_dump($e->getMessage());
    var_dump($e->getFile());
    var_dump($e->getLine());
}
?>
===EOF===
--EXPECTF--
string(14) "Test-Exception"
string(%d) "%sphp_exceptions_basic.php"
int(5)
===EOF===
