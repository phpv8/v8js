--TEST--
Test V8::executeString() : Get constructor method
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	function __construct() {
		echo "called constructor: ";
		var_dump(func_get_args());
	}
}

$v8 = new V8JS();
$v8->foo = new Foo(23);

$js = <<<EOF
var_dump(PHP.foo.constructor);

c = PHP.foo.constructor;
bar = new c(42);

EOF
;
$v8->executeString($js);

?>
===EOF===
--EXPECTF--
called constructor: array(1) {
  [0]=>
  int(23)
}
object(Closure)#%d {
    function Foo() { [native code] }
}
called constructor: array(1) {
  [0]=>
  int(42)
}
===EOF===
