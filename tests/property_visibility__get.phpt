--TEST--
Test V8::executeString() : Property visibility __get
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";

	public function __get($key)
	{
		var_dump($key);
		return 42;
	}
}

$js = new V8Js();

$js->foo = new Foo();

$script = <<<END

var_dump(PHP.foo.unknownBar);
var_dump(PHP.foo.privBar);
var_dump(PHP.foo.protBar);
var_dump(PHP.foo.pubBar);

END;

$js->executeString($script);
?>
===EOF===
--EXPECT--
string(10) "unknownBar"
int(42)
string(7) "privBar"
int(42)
string(7) "protBar"
int(42)
string(6) "pubBar"
===EOF===
