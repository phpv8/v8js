--TEST--
Test V8::executeString() : Property visibility - has property
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

var_dump(PHP.foo.hasOwnProperty("privBar"));
var_dump(PHP.foo.hasOwnProperty("protBar"));
var_dump(PHP.foo.hasOwnProperty("pubBar"));
var_dump(PHP.foo.hasOwnProperty("unknownBar"));

PHP.foo.privBar = 23;
PHP.foo.protBar = 23;
PHP.foo.pubBar = 23;
PHP.foo.unknownBar = 23;

var_dump(PHP.foo.hasOwnProperty("privBar"));
var_dump(PHP.foo.hasOwnProperty("protBar"));
var_dump(PHP.foo.hasOwnProperty("pubBar"));
var_dump(PHP.foo.hasOwnProperty("unknownBar"));

END;

$js->executeString($script);

?>
===EOF===
--EXPECT--
bool(false)
bool(false)
bool(true)
bool(false)
bool(true)
bool(true)
bool(true)
bool(true)
===EOF===
