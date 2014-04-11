--TEST--
Test V8::executeString() : Property visibility __set
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";

	public function __set($name, $value) {
		echo "$name <- $value\n";
	}

	public function dump() {
		var_dump($this->privBar);
		var_dump($this->protBar);
		var_dump($this->pubBar);
		var_dump(isset($this->unknownBar));
		var_dump(isset($this->phpBar));
	}
}

$js = new V8Js();

$js->foo = new Foo();
$js->foo->protBar = 'piet';
$js->foo->phpBar = 'phpValue';

$script = <<<END

PHP.foo.privBar = 'jsPriv';
PHP.foo.protBar = 'jsProt';
PHP.foo.pubBar = 'jsPub';
PHP.foo.unknownBar = 'jsUnknown';

var_dump(PHP.foo.privBar);
var_dump(PHP.foo.protBar);
var_dump(PHP.foo.pubBar);
var_dump(PHP.foo.unknownBar);
var_dump(PHP.foo.phpBar);

END;

$js->executeString($script);
$js->foo->dump();
?>
===EOF===
--EXPECT--
protBar <- piet
phpBar <- phpValue
privBar <- jsPriv
protBar <- jsProt
unknownBar <- jsUnknown
NULL
NULL
string(5) "jsPub"
NULL
NULL
string(7) "privBar"
string(7) "protBar"
string(5) "jsPub"
bool(false)
bool(false)
===EOF===
