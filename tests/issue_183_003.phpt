--TEST--
Test V8::executeString() : Method access on derived classes (V8Js methods)
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
}

$JS = <<< EOT
var_dump(typeof PHP.hello);
var_dump(typeof PHP.executeString);
var_dump(typeof PHP.compileString);
var_dump(typeof PHP.executeScript);
var_dump(typeof PHP.checkString);
var_dump(typeof PHP.getPendingException);
var_dump(typeof PHP.setModuleNormaliser);
var_dump(typeof PHP.setModuleLoader);
var_dump(typeof PHP.registerExtension);
var_dump(typeof PHP.getExtensions);
var_dump(typeof PHP.setTimeLimit);
var_dump(typeof PHP.setMemoryLimit);

try {
    PHP.setTimeLimit(100);
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
string(8) "function"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(9) "undefined"
string(6) "caught"
===EOF===
