--TEST--
Test V8::executeString() : Property visibility
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";
}

$js = new V8Js();

$js->foo = new Foo();

$script = <<<END

var_dump(PHP.foo.privBr);
var_dump(PHP.foo.protBar);
var_dump(PHP.foo.pubBar);

END;

$js->executeString($script);
?>
===EOF===
--EXPECT--
NULL
NULL
string(6) "pubBar"
===EOF===
