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
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}

try {
	$a->executeString("PHP.myobj.mytest('arg1', 'arg2', 'arg3', 'extra_arg');", "test5.js");
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}

// Array / Object
try {
//	date_default_timezone_set("UTC");
	$a->executeString("date = new Date('September 8, 1975 09:00:00'); PHP.print(date); PHP.myobj.mytest(date, PHP.myobj, new Array(1,2,3));", "test6.js");
} catch (V8JsScriptException $e) {
	var_dump($e);
}

try {
	$a->executeString("PHP.myobj.mytest(PHP.myobj, new Array(1,2,3), new Array('foo', 'bar', PHP.myobj));", "test7.js");
} catch (V8JsScriptException $e) {
	var_dump($e);
}
