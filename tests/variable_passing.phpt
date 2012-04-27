--TEST--
Test V8::executeString() : simple variables passed from PHP
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
	var a = 'From PHP: ' + PHP.somevar;
	PHP.somevar = 'changed in JS!'; // Should not change..

	dump(PHP.myarray);

	return a;
}
print(test() + "\\n");
print(PHP.myinteger + "\\n");
print(PHP.myfloat + "\\n");
EOT;

$a = new V8Js();
$a->somevar = "From PHP with love!"; 
$a->myinteger = 123;
$a->myfloat = 3.14;
$a->_SERVER = $_SERVER;
$a->GLOBALS = $GLOBALS;
$a->myarray = array(
	'a' => 'value for key A',
	'b' => 'value for key B',
	'c' => 'value for key C',
	'd' => 'value for key D',
);

$a->executeString($JS, "test.js");

// Check that variable has not been modified
var_dump($a->somevar);
?>
===EOF===
--EXPECT--
a => value for key A
b => value for key B
c => value for key C
d => value for key D
From PHP: From PHP with love!
123
3.14
string(19) "From PHP with love!"
===EOF===
