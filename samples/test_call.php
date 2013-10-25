<?php

class Foo {
	var $bar = 'foobar';

	function MyOwnFunc() {}

	function __Get($name) {
		echo "Called __get(): ";
		var_dump($name);
		return "xyz";
	}
	function __Set($name, $args) {
		echo "Called __set(): ";
		var_dump($name, $args);
	}
	function __Isset($name) {
		echo "Called __isset(): ";
		var_dump($name);
		return true;
	}
	function __call($name, $args) {
		echo "Called __call(): ";
		var_dump($name, $args);
	}
	function __Invoke($name, $arg1, $arg2) {
		echo "Called __invoke(): ";
		var_dump(func_get_args());
		return 'foobar';
	}
	function __toString() {
		echo "Called __tostring: ";
		return $this->bar;
	}
}

$blaa = new V8Js();
$blaa->obj = new Foo;

try {
  echo "__invoke()\n";
  $blaa->executeString("var_dump(PHP.obj('arg1','arg2','arg3'));", "invoke_test1 #1.js");
  echo "------------\n";

  echo " __invoke() with new\n";
  $blaa->executeString("myobj = new PHP.obj('arg1','arg2','arg3'); var_dump(myobj);", "invoke_test2 #2.js");
  echo "------------\n";

  echo " __tostring()\n";
  $blaa->executeString('print(PHP.obj + "\n");', "tostring_test #3.js");
  echo "------------\n";

  echo " __isset() not called with existing property\n";
  $blaa->executeString('if ("bar" in PHP.obj) print("bar exists\n");', "isset_test1 #4.js");
  echo "------------\n";

  echo " __isset() with non-existing property\n";
  $blaa->executeString('if (!("foobar" in PHP.obj)) print("foobar does not exist\n"); else print("We called __isset and it said yes!\n");', "isset_test2 #5.js");
  echo "------------\n";

  echo " __get() not called with existing property\n";
  $blaa->executeString('var_dump(PHP.obj.bar);', "get_test1 #6.js");
  echo "------------\n";

  echo " __get() with non-existing property\n";
  $blaa->executeString('var_dump(PHP.obj.foo);', "get_test2 #7.js");
  echo "------------\n";

  echo " __call()\n";
  $blaa->executeString('PHP.obj.foo(1,2,3);', "call_test1 #8.js");
  echo "------------\n";

} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
