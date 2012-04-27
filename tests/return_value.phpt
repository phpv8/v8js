--TEST--
Test V8::executeString() : Return values
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
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
--EXPECT--
NULL
object(V8Object)#3 (2) {
  ["mytest"]=>
  object(V8Function)#4 (0) {
  }
  ["foo"]=>
  string(8) "ORIGINAL"
}
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
object(DateTime)#3 (3) {
  ["date"]=>
  string(19) "1975-09-08 09:00:00"
  ["timezone_type"]=>
  int(1)
  ["timezone"]=>
  string(6) "+02:00"
}
int(1234567890)
float(123.456789)
string(11) "some string"
bool(true)
bool(false)
===EOF===
