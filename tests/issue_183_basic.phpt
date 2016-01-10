--TEST--
Test V8::executeString() : Method access on derived classes
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
PHP.hello();
EOT;

$v8 = new Foo();
$v8->executeString($JS);

?>
===EOF===
--EXPECT--
Hello World
===EOF===
