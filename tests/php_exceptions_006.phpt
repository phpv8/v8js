--TEST--
Test V8::executeString() : PHP Exception handling (JS throws normal PHP-object)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
    function getNonExceptionObject() {
        return new \Foo();
    }
}

$v8 = new V8Js();
$v8->foo = new \Foo();

$JS = <<< EOT
var ex = PHP.foo.getNonExceptionObject();
print("after getNonExceptionObject\\n");
throw ex;
print("after throw\\n");
EOT;

try {
    $v8->executeString($JS, 'php_exceptions_006');
}
catch(V8JsScriptException $e) {
    echo "Got V8JsScriptException\n";
    var_dump($e->getMessage());
    // previous exception should be NULL, as it is *not* a php exception
    var_dump($e->getPrevious());
}
?>
===EOF===
--EXPECTF--
after getNonExceptionObject
Got V8JsScriptException
string(34) "php_exceptions_006:3: [object Foo]"
NULL
===EOF===
