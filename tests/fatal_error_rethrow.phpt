--TEST--
Test V8::executeString() : Fatal Error rethrowing
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = new V8Js();

$js->foo = function() {
	$bar = null;
	$bar->bar();
};

$script = <<<END

PHP.foo();

END;

$js->executeString($script);

?>
===EOF===
--EXPECTF--
Fatal error: Call to a member function bar() on %s in %s%efatal_error_rethrow.php on line 7
