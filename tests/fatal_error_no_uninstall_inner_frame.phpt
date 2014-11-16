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

Fatal error: Call to a member function foo() on %s in %s%efatal_error_no_uninstall_inner_frame.php on line 15
