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
Fatal error: Uncaught Error: Call to a member function bar() on %s in %s%efatal_error_rethrow.php:7
Stack trace:
#0 [internal function]: {closure}()
#1 [internal function]: Closure->__invoke()
#2 %s%efatal_error_rethrow.php(16): V8Js->executeString(%s)
#3 {main}
  thrown in %s%efatal_error_rethrow.php on line 7
