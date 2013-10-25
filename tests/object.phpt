--TEST--
Test V8::executeString() : Object passed from PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
function dump(a)
{
	for (var i in a) { 
		var val = a[i];
		print(i + ' => ' + val + "\\n");
	}
}
function test()
{
	dump(PHP.myobj);
	PHP.myobj.foo = 'CHANGED';
	PHP.myobj.mytest();
}
test();
print(PHP.myobj.foo + "\\n");
EOT;

// Test class
class Testing
{
	public $foo = 'ORIGINAL';
	private $my_private = 'arf'; // Should not show in JS side
	protected $my_protected = 'argh'; // Should not show in JS side

	function mytest() { echo 'Here be monsters..', "\n"; }
}

$a = new V8Js();
$a->myobj = new Testing();
$a->executeString($JS, "test.js");

// Check that variable has not been modified
var_dump($a->myobj->foo);
?>
===EOF===
--EXPECT--
mytest => function () { [native code] }
$foo => ORIGINAL
Here be monsters..
CHANGED
string(7) "CHANGED"
===EOF===
