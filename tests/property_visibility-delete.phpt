--TEST--
Test V8::executeString() : Property visibility - delete
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";

	public function dump($a)
	{
		var_dump(@$this->$a);
	}
}

$js = new V8Js();
$js->foo = new Foo();

$script = <<<END

var_dump(PHP.foo.privBar);
delete PHP.foo.privBar;
var_dump(PHP.foo.privBar);

PHP.foo.privBar = 42;

var_dump(PHP.foo.privBar);
delete PHP.foo.privBar;
var_dump(PHP.foo.privBar);

var_dump(PHP.foo.protBar);
delete PHP.foo.protBar;
var_dump(PHP.foo.protBar);

var_dump(PHP.foo.pubBar);
delete PHP.foo.pubBar;
var_dump(PHP.foo.pubBar);

END;

$js->foo->dump('privBar');
$js->foo->dump('protBar');
$js->foo->dump('pubBar');

echo "--- JS ---\n";
$js->executeString($script);

echo "--- PHP ---\n";
$js->foo->dump('privBar');
$js->foo->dump('protBar');
$js->foo->dump('pubBar');

?>
===EOF===
--EXPECT--
string(7) "privBar"
string(7) "protBar"
string(6) "pubBar"
--- JS ---
NULL
NULL
int(42)
NULL
NULL
NULL
string(6) "pubBar"
NULL
--- PHP ---
string(7) "privBar"
string(7) "protBar"
NULL
===EOF===
