--TEST--
Test V8::executeString() : Fatal Error with recursive executeString calls
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = new V8Js();

$js->baz = function() {
	$bar = null;
	$bar->bar();
};

$js->bar = function() {
	global $js;
	$js->executeString("PHP.baz();");
};

$js->foo = function() {
	global $js;
	$js->executeString("PHP.bar();");
};

$js->nofail = function() {
	echo "foo\n";
};

$js->executeString("PHP.nofail();");
$js->executeString("PHP.nofail(); PHP.foo();");

?>
===EOF===
--EXPECTF--
foo
foo

Fatal error: Uncaught Error: Call to a member function bar() on null in %s%efatal_error_recursive.php:7
Stack trace:
#0 [internal function]: {closure}()
#1 [internal function]: Closure->__invoke()
#2 %s%efatal_error_recursive.php(12): V8Js->executeString('PHP.baz();')
#3 [internal function]: {closure}()
#4 [internal function]: Closure->__invoke()
#5 %s%efatal_error_recursive.php(17): V8Js->executeString('PHP.bar();')
#6 [internal function]: {closure}()
#7 [internal function]: Closure->__invoke()
#8 %s%efatal_error_recursive.php(25): V8Js->executeString('PHP.nofail(); P...')
#9 {main}
  thrown in %s%efatal_error_recursive.php on line 7
