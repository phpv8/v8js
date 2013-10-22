--TEST--
Test V8::executeString() : Object passing PHP > JS > PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Bar {
  function sayHello() {
	echo "Hello\n";
  }
}

class Foo {
  function getBar() {
	return new Bar();
  }

  function callMulti($list) {
	foreach($list as $x) {
	  echo get_class($x)."\n";  // V8Object vs. Bar
	  $x->sayHello();
	}
  }

  function callSingle($inst) {
	echo get_class($inst)."\n";
	$inst->sayHello();
  }
}

$v8 = new V8Js();
$v8->foo = new Foo();

$JS = <<< EOF
var obj = PHP.foo.getBar();
PHP.foo.callMulti([obj]);
PHP.foo.callMulti([obj]);
PHP.foo.callSingle(obj);
PHP.foo.callSingle(obj);

obj = {};
obj.sayHello = function() {
	print("JavaScript Hello\\n");
};

PHP.foo.callMulti([obj]);
PHP.foo.callMulti([obj]);
PHP.foo.callSingle(obj);
PHP.foo.callSingle(obj);
EOF;

$v8->executeString($JS);

?>
===EOF===
--EXPECT--
Bar
Hello
Bar
Hello
Bar
Hello
Bar
Hello
V8Object
JavaScript Hello
V8Object
JavaScript Hello
V8Object
JavaScript Hello
V8Object
JavaScript Hello
===EOF===
