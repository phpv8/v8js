--TEST--
Test V8::executeString() : Calling methods of object passed from PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
date.timezone=UTC
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

  function mydatetest(DateTime $date, $b) {
    $date->setTimeZone(new DateTimeZone(ini_get('date.timezone')));
    echo $date->format(DateTime::RFC1123), "\n";
    var_dump($b);
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
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}

try {
	$a->executeString("PHP.myobj.mytest('arg1', 'arg2', 'arg3', 'extra_arg');", "test5.js");
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}

try {
	echo "\nTEST: Javascript Date -> PHP DateTime\n";
	echo "======================================\n";
	$a->executeString("date = new Date('September 8, 1975 09:00:00 GMT'); print(date.toUTCString() + '\\n'); PHP.myobj.mydatetest(date, 'foo');", "test6.js");
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}

// Array / Object
try {
	$a->executeString("PHP.myobj.mytest(PHP.myobj, new Array(1,2,3), new Array('foo', 'bar', PHP.myobj));", "test7.js");
} catch (V8JsScriptException $e) {
	var_dump($e);
}

// Type safety
// this is illegal, but shouldn't crash!
try {
	$a->executeString("PHP.myobj.mytest.call({})", "test8.js");
} catch (V8JsScriptException $e) {
	echo "exception: ", $e->getMessage(), "\n";
}

?>
===EOF===
--EXPECTF--
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
Mon, 08 Sep 1975 09:00:00 GMT
Mon, 08 Sep 1975 09:00:00 +0000
string(3) "foo"
array(3) {
  [0]=>
  object(Testing)#%d (3) {
    ["foo"]=>
    string(8) "ORIGINAL"
    ["my_private":"Testing":private]=>
    string(3) "arf"
    ["my_protected":protected]=>
    string(4) "argh"
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
    object(Testing)#%d (3) {
      ["foo"]=>
      string(8) "ORIGINAL"
      ["my_private":"Testing":private]=>
      string(3) "arf"
      ["my_protected":protected]=>
      string(4) "argh"
    }
  }
}
exception: test8.js:1: TypeError: Illegal invocation
===EOF===
