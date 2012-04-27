<?php

$JS = <<< EOT
function test(passed)
{
	return passed;
}
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
var_dump($a->executeString($JS, "test.js"));
var_dump($a->executeString("test(PHP.myobj);", "test1.js"));
var_dump($a->executeString("test(new Array(1,2,3));", "test2.js"));
var_dump($a->executeString("test(new Array('foo', 'bar'));", "test3.js"));
var_dump($a->executeString("test(new Array('foo', 'bar'));", "test3.js"));
var_dump($a->executeString("test(new Date('September 8, 1975 09:00:00'));", "test4.js"));
var_dump($a->executeString("test(1234567890);", "test5.js"));
var_dump($a->executeString("test(123.456789);", "test6.js"));
var_dump($a->executeString("test('some string');", "test7.js"));
var_dump($a->executeString("test(true);", "test8.js"));
var_dump($a->executeString("test(false);", "test9.js"));
?>
===EOF===
