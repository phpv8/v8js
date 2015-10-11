--TEST--
Test V8::executeString() : PHP Exception handling (JS throw PHP-exception)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
    function getException() {
        return new \Exception("Test-Exception");
    }
}

$v8 = new V8Js();
$v8->foo = new \Foo();

$JS = <<< EOT
var ex = PHP.foo.getException();
print("after getException\\n");
throw ex;
print("after throw\\n");
EOT;

try {
    $v8->executeString($JS, 'php_exceptions_005');
}
catch(V8JsScriptException $e) {
    echo "Got V8JsScriptException\n";
    var_dump($e->getMessage());
    var_dump($e->getPrevious()->getMessage());
}
?>
===EOF===
--EXPECTF--
after getException
Got V8JsScriptException
string(%d) "php_exceptions_005:3: Exception: Test-Exception in %s
Stack trace:
#0 [internal function]: Foo->getException()
#1 %s: V8Js->executeString('var ex = PHP.fo...', 'php_exceptions_...')
#2 {main}"
string(14) "Test-Exception"
===EOF===
