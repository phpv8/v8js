--TEST--
Test V8::executeString() : has_property after dispose
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	function callMe($x) {
		var_dump(property_exists($x, 'bla'));
		$this->x = $x;
	}
}

$v8 = new V8Js();
$v8->foo = $foo = new Foo();

$JS = <<< EOT
PHP.foo.callMe({ bla: 23 });

EOT;

$v8->executeString($JS, 'basic.js');
unset($v8);

try {
	var_dump(property_exists($foo->x, 'bla'));
}
catch(V8JsException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECTF--
bool(true)
string(55) "Can't access V8Object after V8Js instance is destroyed!"
===EOF===

