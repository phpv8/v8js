--TEST--
Test V8::executeString() : Fatal Error handler to ignore warnings
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = new V8Js();

$js->foo = function() {
	echo $bar;
	trigger_error('Foo Bar!', E_USER_WARNING);
	echo "blar foo\n";
};

$script = <<<END

PHP.foo();

END;

$js->executeString($script);

?>
===EOF===
--EXPECTF--
Notice: Undefined variable: bar in %s%efatal_error_ignore_non_fatals.php on line 6

Warning: Foo Bar! in %s%efatal_error_ignore_non_fatals.php on line 7
blar foo
===EOF===