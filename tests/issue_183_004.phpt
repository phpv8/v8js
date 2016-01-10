--TEST--
Test V8::executeString() : Method access on derived classes (overridden V8Js methods)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo extends \V8Js
{
    public function hello()
    {
        print("Hello World\n");
    }

    public function executeString($script, $identifier = NULL, $flags = NULL, $time_limit = NULL, $memory_limit = NULL)
    {
        var_dump("executeString");
        return parent::executeString($script);
    }
}

$JS = <<< EOT
var_dump(typeof PHP.hello);
var_dump(typeof PHP.executeString);

try {
    PHP.executeString('print("blar")');
}
catch(e) {
    var_dump('caught');
}
EOT;

$v8 = new Foo();
$v8->executeString($JS);

?>
===EOF===
--EXPECTF--
string(13) "executeString"
string(8) "function"
string(9) "undefined"
string(6) "caught"
===EOF===
