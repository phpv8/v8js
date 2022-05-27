--TEST--
Test V8::executeString() : Exception propagation test 3
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $v8 = NULL;

	public function __construct()
	{
		$this->v8 = new V8Js(null, array(), array(), false);
		$this->v8->foo = $this;
		$this->v8->executeString('function foobar() { throw new SyntaxError(); }', 'throw_1');
		$this->v8->executeString('try { foobar(); } catch (e) { print(e + " caught in JS!\n"); }', 'trycatch1');
		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " caught via PHP callback!\n"); }', 'trycatch2');
	}

	public function bar()
	{
		echo "To Bar!\n";
		$this->v8->executeString('throw new Error();', 'throw_2');
	}
}
    
try {
	$foo = new Foo();
} catch (V8JsScriptException $e) {
	echo "PHP Exception: ", $e->getMessage(), "\n";
}
?>
===EOF===
--EXPECTF--
Deprecated: V8Js::__construct(): Disabling exception reporting is deprecated, $report_uncaught_exceptions != true in %s%eexception_propagation_3.php on line 8
SyntaxError caught in JS!
To Bar!
Error caught via PHP callback!
===EOF===
