--TEST--
Test V8::executeString() : Use after dispose
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	function callMe($x) {
		var_dump($x);
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
	var_dump($foo->x);
}
catch(V8JsException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECTF--
object(V8Object)#%d (1) {
  ["bla"]=>
  int(23)
}
object(V8Object)#%d (0) {
}
string(55) "Can't access V8Object after V8Js instance is destroyed!"
===EOF===
