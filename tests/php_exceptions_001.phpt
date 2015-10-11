--TEST--
Test V8::executeString() : PHP Exception handling (repeated)
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

for($i = 0; $i < 5; $i ++) {
    var_dump($i);
    try {
        $v8->executeString($JS);
    } catch (Exception $e) {
        var_dump($e->getMessage());
    }
}
?>
===EOF===
--EXPECTF--
int(0)
string(14) "Test-Exception"
int(1)
string(14) "Test-Exception"
int(2)
string(14) "Test-Exception"
int(3)
string(14) "Test-Exception"
int(4)
string(14) "Test-Exception"
===EOF===
