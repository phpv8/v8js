--TEST--
Test V8::executeString() : Property visibility - enumerate
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $privBar = "privBar";
	protected $protBar = "protBar";
	public $pubBar = "pubBar";

	public function dump() {
		foreach($this as $key => $value) {
			echo("$key => ");
			var_dump($value);
		}
	}
}

$js = new V8Js();
$js->foo = new Foo();

$script = <<<END

for(var key in PHP.foo) {
	if(PHP.foo.hasOwnProperty(key)) {
		var_dump(key);

		if(key[0] === '$') {
			var_dump(PHP.foo[key]);
		}
		else {
			var_dump("function");
		}
	}
}

END;

$js->executeString($script);

echo "--- PHP ---\n";
$js->foo->dump();
?>
===EOF===
--EXPECT--
string(4) "dump"
string(8) "function"
string(7) "$pubBar"
string(6) "pubBar"
--- PHP ---
privBar => string(7) "privBar"
protBar => string(7) "protBar"
pubBar => string(6) "pubBar"
===EOF===
