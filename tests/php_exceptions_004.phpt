--TEST--
Test V8::executeString() : PHP Exception handling (PHP->JS->PHP back propagation)
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
PHP.foo.throwException();
// the exception should abort further execution,
// hence the print must not pop up
print("after throwException\\n");
EOT;

try {
    $v8->executeString($JS, 'php_exceptions_004', V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
}
catch(V8JsScriptException $e) {
    echo "Got V8JsScriptException\n";
    var_dump($e->getPrevious()->getMessage());
}
?>
===EOF===
--EXPECTF--
Got V8JsScriptException
string(14) "Test-Exception"
===EOF===
