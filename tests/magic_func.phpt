--TEST--
Test V8::executeString() : Object with magic functions
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	var $bar = 'foobar';
	var $nullprop = null;

	function Foo() {
		echo "called constructor: ";
		var_dump(func_get_args());
	}

	function MyOwnFunc() {
		echo "called MyOwnFunc\n";
	}

	function __Get($name) {
		echo "Called __get(): ";
		var_dump($name);
		return null;
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
	function __unSet($name) {
		echo "Called __unset(): ";
		var_dump($name);
	}
	function __call($name, $args) {
		echo "Called __call(): ";
		var_dump($name, $args);
		return "call";
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

class Bar {
	function foo($arg1, $arg2, $arg3) {
		echo "Called foo(): ";
		var_dump(func_get_args());
		return "test";
	}
}


$blaa = new V8Js();
$blaa->obj = $obj = new Foo;

try {
  echo "__invoke() [PHP]\n";
  var_dump($obj('arg1','arg2','arg3'));
  echo "__invoke() [JS]\n";
  $blaa->executeString("var_dump(PHP.obj('arg1','arg2','arg3'));", "invoke_test1 #1.js");
  echo "------------\n";

  echo " __invoke() with new [PHP]\n";
  $myobj = new $obj('arg1','arg2','arg3'); $myobj->myownfunc();
  echo " __invoke() with new [JS]\n";
  $blaa->executeString("myobj = new PHP.obj('arg1','arg2','arg3'); myobj.myownfunc();", "invoke_test2 #2.js");
  echo "------------\n";

  echo " __tostring() [PHP]\n";
  echo $obj; echo "\n";
  echo " __tostring() [JS]\n";
  $blaa->executeString('print(PHP.obj + "\n");', "tostring_test #3.js");
  echo "------------\n";

  echo " __isset() not called with existing property [PHP]\n";
  if (isset($obj->bar)) { echo "bar exists\n"; }
  echo " __isset() not called with existing property [JS]\n";
  $blaa->executeString('if ("bar" in PHP.obj) print("bar exists\n");', "isset_test1 #4.js");
  echo "------------\n";

  echo " __isset() with non-existing property [PHP]\n";
  if (!isset($obj->foobar)) { echo "foobar does not exist\n"; } else { echo "We called __isset and it said yes!\n"; }
  echo " __isset() with non-existing property [JS]\n";
  $blaa->executeString('if (!("foobar" in PHP.obj)) print("foobar does not exist\n"); else print("We called __isset and it said yes!\n");', "isset_test2 #5.js");
  echo "------------\n";

  echo " in works like isset [PHP]\n";
  echo "nullprop is ", (isset($obj->nullprop) ? "" : "not "), "set\n";
  echo " in works like isset [JS]\n";
  $blaa->executeString('print("nullprop is ", ("nullprop" in PHP.obj) ? "" : "not ", "set\n");', "isset_test3 #6.js");
  echo "------------\n";

  echo " __get() not called with existing property [PHP]\n";
  var_dump($obj->bar);
  echo " __get() not called with existing property [JS]\n";
  $blaa->executeString('var_dump(PHP.obj.bar);', "get_test1 #7.js");
  echo "------------\n";

  echo " __get() with non-existing property [PHP]\n";
  var_dump($obj->fooish);
  echo " __get() with non-existing property [JS]\n";
  $blaa->executeString('var_dump(PHP.obj.fooish);', "get_test2 #8.js");
  echo "------------\n";

  echo " __unset() with non-existing property [PHP]\n";
  unset($obj->foobar);
  echo " __unset() with non-existing property [JS]\n";
  $blaa->executeString('delete PHP.obj.foobar;', "unset_test1 #9.js");
  echo "------------\n";

  echo " __unset() with existing property [PHP]\n";
  $obj2 = new Foo; unset($obj2->bar);
  echo " __unset() with existing property [JS]\n";
  $blaa->obj2 = new Foo;
  $blaa->executeString('delete PHP.obj2.bar;', "unset_test2 #10.js");
  echo " fetching the unset property [PHP]\n";
  var_dump($obj2->bar);
  echo " fetching the unset property [JS]\n";
  $blaa->executeString('var_dump(PHP.obj2.bar);', "unset_test3 #11.js");
  echo "------------\n";

  echo " __call() [PHP]\n";
  var_dump($obj->fooish(1,2,3));
  echo " __call() [JS]\n";
  # note that 'PHP.obj.fooish(1,2,3)' won't work in JS, we need to use the
  # '__call' pseudo-method.
  $blaa->executeString('var_dump(PHP.obj.__call("fooish", [1,2,3]));', "call_test1 #12.js");
  echo "------------\n";

  # the __call pseudo-method should work in JS even if the PHP class doesn't
  # define an explicit __call magic function.  This makes it always safe to
  # use __call() if you want to be sure that any __call() handlers are invoked
  # (bypassing __get handlers, as is it done in PHP)
  $blaa->obj3 = $obj3 = new Bar;
  echo " __call() w/o handler [PHP]\n";
  var_dump($obj3->foo(1,2,3));
  echo " __call() w/o handler [JS]\n";
  $blaa->executeString('var_dump(PHP.obj3.__call("foo", [1,2,3]));', "call_test2 #13.js");
  echo "------------\n";

  # The Bar object should inherit toString() and hasOwnProperty() methods
  # from Object
  echo " __toString in Bar [PHP]\n";
  var_dump(method_exists( $obj3, '__toString' ));
  echo " toString in Bar [PHP]\n";
  var_dump(method_exists( $obj3, 'toString' ));
  echo " hasOwnProperty in Bar [PHP]\n";
  var_dump(method_exists( $obj3, 'hasOwnProperty' ));
  echo " __toString in Bar [JS]\n";
  $blaa->executeString('var_dump("__toString" in PHP.obj3 && typeof PHP.obj3.__toString == "function");', "inherit_test1 #14.js");
  # use '$toString' if you actually wanted to check for a PHP property
  # named 'toString' in Bar (instead of the inherited JavaScript property)
  echo " toString in Bar [JS]\n";
  $blaa->executeString('var_dump("toString" in PHP.obj3 && typeof PHP.obj3.toString == "function");', "inherit_test1 #15.js");
  # use '$hasOwnProperty' if you actually wanted to check for a PHP property
  # named 'hasOwnProperty' in Bar (instead of the inherited JavaScript property)
  echo " hasOwnProperty in Bar [JS]\n";
  $blaa->executeString('var_dump("hasOwnProperty" in PHP.obj3 && typeof PHP.obj3.hasOwnProperty == "function");', "inherit_test1 #16.js");
  echo "------------\n";

} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
?>
===EOF===
--EXPECT--
called constructor: array(0) {
}
__invoke() [PHP]
Called __invoke(): array(3) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
  [2]=>
  string(4) "arg3"
}
string(6) "foobar"
__invoke() [JS]
Called __invoke(): array(3) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
  [2]=>
  string(4) "arg3"
}
string(6) "foobar"
------------
 __invoke() with new [PHP]
called constructor: array(3) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
  [2]=>
  string(4) "arg3"
}
called MyOwnFunc
 __invoke() with new [JS]
called constructor: array(3) {
  [0]=>
  string(4) "arg1"
  [1]=>
  string(4) "arg2"
  [2]=>
  string(4) "arg3"
}
called MyOwnFunc
------------
 __tostring() [PHP]
Called __tostring: foobar
 __tostring() [JS]
Called __get(): string(7) "valueOf"
Called __tostring: foobar
------------
 __isset() not called with existing property [PHP]
bar exists
 __isset() not called with existing property [JS]
bar exists
------------
 __isset() with non-existing property [PHP]
Called __isset(): string(6) "foobar"
We called __isset and it said yes!
 __isset() with non-existing property [JS]
Called __isset(): string(6) "foobar"
We called __isset and it said yes!
------------
 in works like isset [PHP]
nullprop is not set
 in works like isset [JS]
nullprop is not set
------------
 __get() not called with existing property [PHP]
string(6) "foobar"
 __get() not called with existing property [JS]
string(6) "foobar"
------------
 __get() with non-existing property [PHP]
Called __get(): string(6) "fooish"
NULL
 __get() with non-existing property [JS]
Called __get(): string(6) "fooish"
NULL
------------
 __unset() with non-existing property [PHP]
Called __unset(): string(6) "foobar"
 __unset() with non-existing property [JS]
Called __unset(): string(6) "foobar"
------------
 __unset() with existing property [PHP]
called constructor: array(0) {
}
 __unset() with existing property [JS]
called constructor: array(0) {
}
 fetching the unset property [PHP]
Called __get(): string(3) "bar"
NULL
 fetching the unset property [JS]
Called __get(): string(3) "bar"
NULL
------------
 __call() [PHP]
Called __call(): string(6) "fooish"
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(4) "call"
 __call() [JS]
Called __call(): string(6) "fooish"
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(4) "call"
------------
 __call() w/o handler [PHP]
Called foo(): array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(4) "test"
 __call() w/o handler [JS]
Called foo(): array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
string(4) "test"
------------
 __toString in Bar [PHP]
bool(false)
 toString in Bar [PHP]
bool(false)
 hasOwnProperty in Bar [PHP]
bool(false)
 __toString in Bar [JS]
bool(false)
 toString in Bar [JS]
bool(true)
 hasOwnProperty in Bar [JS]
bool(true)
------------
===EOF===
