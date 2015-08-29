--TEST--
Test V8::executeString() : Fatal Error handler not to uninstall on inner frames
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = new V8Js();

$js->bar = function() {
	echo "nothing.\n";
};

$js->foo = function() {
	global $js;
	// call to JS context, this must not touch the error handling context
	$js->executeString("PHP.bar();");

	$bar = null;
	$bar->foo();
};

$js->executeString("PHP.foo();");

?>
===EOF===
--EXPECTF--
nothing.

Fatal error: Uncaught Error: Call to a member function foo() on null in %s%efatal_error_no_uninstall_inner_frame.php:15
Stack trace:
#0 [internal function]: {closure}()
#1 [internal function]: Closure->__invoke()
#2 %s%efatal_error_no_uninstall_inner_frame.php(18): V8Js->executeString('PHP.foo();')
#3 {main}
  thrown in %s%efatal_error_no_uninstall_inner_frame.php on line 15
