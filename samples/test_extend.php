<?php

// Test class
class Testing extends V8Js
{
	public $foo = 'ORIGINAL';
	private $my_private = 'arf'; // Should not show in JS side
	protected $my_protected = 'argh'; // Should not show in JS side

	public function mytest($a, $b, $c = NULL)
	{
		var_dump(func_get_args());
	}
}

$a = new Testing();
echo $a;

try {
	$a->executeString("PHP.mytest(PHP.foo, PHP.my_private, PHP.my_protected);", "test7.js");
} catch (V8JsScriptException $e) {
	var_dump($e);
}
