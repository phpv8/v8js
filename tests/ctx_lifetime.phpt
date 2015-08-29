--TEST--
Test V8::executeString() : Testing lifespan of V8Js context objects
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo
{
	function hello() {
		echo "Hello!\n";
	}
}

class Testing
{
	function onectx()
	{
		$v8js = new V8Js();
		$v8js->foo = new Foo;
		return $v8js->executeString("({ bar: 23, hello: function() { PHP.foo.__call('hello',[]); } })");
		// $v8js will be dereferenced here, but the result escapes.
	}
}

$t = new Testing();

$a = $t->onectx();
/* $a is no longer valid, since the associated V8Js() object has been
 * destroyed.  Instead the property access will throw. */
var_dump($a);

try {
  var_dump($a->bar);
}
catch(Exception $e) {
  var_dump($e->getMessage());
}

$a->hello();

?>
===EOF===
--EXPECTF--
object(V8Object)#%d (0) {
}
string(55) "Can't access V8Object after V8Js instance is destroyed!"

Fatal error: Uncaught V8JsException: Can't access V8Object after V8Js instance is destroyed! in %s%etests%ectx_lifetime.php:35
Stack trace:
#0 {main}
  thrown in %s%etests%ectx_lifetime.php on line 35
