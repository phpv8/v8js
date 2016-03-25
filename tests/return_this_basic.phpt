--TEST--
Test V8::executeString() : return $this (aka fluent setters)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $foo;
	private $bar;

	public function setFoo($value)
	{
		$this->foo = $value;
		return $this;
	}

	public function setBar($value)
	{
		$this->bar = $value;
		return $this;
	}
}

$v8 = new V8Js();
$v8->theFoo = new Foo();

$v8->executeString(<<<EOJS
	var a = PHP.theFoo.setFoo(23);
	var b = a.setBar(42);

	var_dump(PHP.theFoo === a);
	var_dump(PHP.theFoo === b);
EOJS
);

var_dump($v8->theFoo);

?>
===EOF===
--EXPECTF--
bool(true)
bool(true)
object(Foo)#%d (2) {
  ["foo":"Foo":private]=>
  int(23)
  ["bar":"Foo":private]=>
  int(42)
}
===EOF===
