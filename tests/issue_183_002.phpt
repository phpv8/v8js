--TEST--
Test V8::executeString() : Method access on derived classes (private)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo extends \V8Js
{
	private function hello()
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
--EXPECTF--
Fatal error: Uncaught exception 'V8JsScriptException' with message 'V8Js::compileString():1: TypeError: %s is not a function' in %s
Stack trace:
#0 %s: V8Js->executeString('PHP.hello();')
#1 {main}
  thrown in %s on line 16
