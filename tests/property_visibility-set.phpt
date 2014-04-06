--TEST--
Test V8::executeString() : Property visibility - set
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";

	public function dump() {
		var_dump($this->privBar);
		var_dump($this->protBar);
		var_dump($this->pubBar);
	}
}

$js = new V8Js();
$js->foo = new Foo();

$script = <<<END

PHP.foo.privBar = 'jsPriv';
PHP.foo.protBar = 'jsProt';
PHP.foo.pubBar = 'jsPub';

var_dump(PHP.foo.privBar);
var_dump(PHP.foo.protBar);
var_dump(PHP.foo.pubBar);

END;

$js->executeString($script);
$js->foo->dump();
?>
===EOF===
--EXPECT--
string(6) "jsPriv"
string(6) "jsProt"
string(5) "jsPub"
string(7) "privBar"
string(7) "protBar"
string(5) "jsPub"
===EOF===
