--TEST--
Test V8::executeString() : Calling methods of object passed from PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

// Test class
class Testing
{
	public $foo = 'ORIGINAL';
	private $my_private = 'arf'; // Should not show in JS side
	protected $my_protected = 'argh'; // Should not show in JS side

	function mytest($a, $b, $c = NULL)
	{
		var_dump(func_get_args());
	}
}

$a = new V8Js();
$a->myobj = new Testing();

$a->executeString("PHP.myobj.mytest('arg1', 'arg2');", "test1.js");
$a->executeString("PHP.myobj.mytest(true, false, 1234567890);", "test2.js");
$a->executeString("PHP.myobj.mytest(3.14, 42, null);", "test3.js");

// Invalid parameters
try {
	$a->executeString("PHP.myobj.mytest();", "test4.js");
} catch (V8JsException $e) {
	echo $e->getMessage(), "\n";
}

try {
	$a->executeString("PHP.myobj.mytest('arg1', 'arg2', 'arg3', 'extra_arg');", "test5.js");
} catch (V8JsException $e) {
	echo $e->getMessage(), "\n";
}

try {
	date_default_timezone_set("UTC");
	echo "\nTEST: Javascript Date -> PHP DateTime\n";
	echo "======================================\n";
	$a->executeString("date = new Date('September 8, 1975 09:00:00'); print(date + '\\n'); PHP.myobj.mytest(date, 'foo');", "test6.js");
} catch (V8JsException $e) {
	echo $e->getMessage(), "\n";
}

// Array / Object
try {
	$a->executeString("PHP.myobj.mytest(PHP.myobj, new Array(1,2,3), new Array('foo', 'bar', PHP.myobj));", "test7.js");
} catch (V8JsException $e) {
	var_dump($e);
}

?>
===EOF===
--EXPECT--
array(2) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
}
array(3) {
  [0]=>
  bool(true)
  [1]=>
  bool(false)
  [2]=>
  int(1234567890)
}
array(3) {
  [0]=>
  float(3.14)
  [1]=>
  int(42)
  [2]=>
  NULL
}
test4.js:1: TypeError: Testing::mytest() expects at least 2 parameters, 0 given
array(4) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
  [2]=>
  string(4) "arg3"
  [3]=>
  string(9) "extra_arg"
}

TEST: Javascript Date -> PHP DateTime
======================================
Mon Sep 08 1975 09:00:00 GMT+0200 (EET)
array(2) {
  [0]=>
  object(DateTime)#4 (3) {
    ["date"]=>
    string(19) "1975-09-08 09:00:00"
    ["timezone_type"]=>
    int(1)
    ["timezone"]=>
    string(6) "+02:00"
  }
  [1]=>
  string(3) "foo"
}
array(3) {
  [0]=>
  object(V8Object)#4 (2) {
    ["mytest"]=>
    object(V8Function)#6 (0) {
    }
    ["foo"]=>
    string(8) "ORIGINAL"
  }
  [1]=>
  array(3) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [2]=>
    int(3)
  }
  [2]=>
  array(3) {
    [0]=>
    string(3) "foo"
    [1]=>
    string(3) "bar"
    [2]=>
    object(V8Object)#5 (2) {
      ["mytest"]=>
      object(V8Function)#6 (0) {
      }
      ["foo"]=>
      string(8) "ORIGINAL"
    }
  }
}
===EOF===
