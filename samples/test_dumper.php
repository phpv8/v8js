<?php

class Foo {
	var $foo = 'bar';
	var $true = true;
	var $false = false;
	var $bar = array(1,2,3,1.23456789);
	var $ass = array("life" => 42, "foo" => "bar");
	function __set($name, $value)
	{
		echo "I'm setter!\n";
		var_dump($name, $value);
	}
	function __get($name)
	{
		echo "I'm getter!\n";
		var_dump($name);
	}
	function __call($name, $args)
	{
		echo "I'm caller!\n";
		var_dump($name, $args);
	}
}

$a = new V8Js();
$obj = new Foo;
$a->arr = array("foobar" => $obj);

$JS = <<< 'EOF'
  var example = new Object;
  example.foo = function () {
    print("this is foo");
  }
  example.bar = function () {
    print("this is bar");
  }
  example.__noSuchMethod__ = function (id, args) {
    print("tried to handle unknown method " + id);
    if (args.length != 0)
      print("it had arguments: " + args);
  }
  example.foo();        // alerts "this is foo"
  example.bar();        // alerts "this is bar"
  example.grill();      // alerts "tried to handle unknown method grill"
  example.ding("dong"); // alerts "tried to handle unknown method ding"
EOF;

try {
  $a->executeString("var myarr = new Array(); myarr[0] = 'foo'; myarr[1] = 'bar'; var_dump(myarr); var_dump(new Date('September 8, 1975 09:00:00'))", "call_test1.js");
  $a->executeString("var_dump(PHP.arr.foobar.bar);", "call_test2.js");
  $a->executeString("var_dump(PHP.arr.foobar.bar[0]);", "call_test3.js");
  $a->executeString("var_dump(var_dump(PHP.arr));", "call_test4.js");
  $a->executeString("var patt1=/[^a-h]/g; var_dump(patt1);", "call_test5.js");
  $a->executeString("var_dump(Math.PI, Infinity, null, undefined);", "call_test6.js");
//  $a->executeString($JS);
} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
