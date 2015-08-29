--TEST--
Test V8::executeString() : Fatal Error handler must be uninstalled when leaving outermost frame
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
};

$js->executeString("PHP.foo();");

// V8JS handler must be removed, just throw error ...
// if V8JS handler is not removed, it should trigger segfault in V8 :)
$bar = null;
$bar->foo();

?>
===EOF===
--EXPECTF--
nothing.

Fatal error: Uncaught Error: Call to a member function foo() on null in %s%efatal_error_uninstall_in_first_frame.php:20
Stack trace:
#0 {main}
  thrown in %s%efatal_error_uninstall_in_first_frame.php on line 20
