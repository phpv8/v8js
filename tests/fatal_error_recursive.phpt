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

Fatal error: Call to a member function bar() on %s in %s%efatal_error_recursive.php on line 7
