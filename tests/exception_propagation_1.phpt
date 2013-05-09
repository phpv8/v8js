--TEST--
Test V8::executeString() : Exception propagation test 1
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $v8 = NULL;

	public function __construct()
	{
		$this->v8 = new V8Js();
		$this->v8->foo = $this;
		$this->v8->executeString('fooobar', 'throw_0');
		var_dump($this->v8->getPendingException());
		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " caught!\n"); }', 'trycatch1');
		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " caught!\n"); }', 'trycatch2');
	}

	public function bar()
	{
		echo "To Bar!\n";
		$this->v8->executeString('throw new Error();', 'throw_1');
	}
}
    
try {
	$foo = new Foo();
} catch (V8JsScriptException $e) {
	echo "PHP Exception: ", $e->getMessage(), "\n"; //var_dump($e);
}
?>
===EOF===
--EXPECTF--
PHP Exception: throw_0:1: ReferenceError: fooobar is not defined
===EOF===
