--TEST--
Test V8::setExceptionFilter() : Filter handling on exception in setModuleNormaliser
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$v8->setModuleNormaliser(function ($path) {
	throw new Error('blarg');
});
$v8->setModuleLoader(function ($path) {
	throw new Error('moep');
});

$v8->setExceptionFilter(function (Throwable $ex) {
	echo "exception filter called.\n";
	return $ex->getMessage();
});

$v8->executeString('
	try {
		require("file");
	} catch(e) {
		var_dump(e);
	}
', null, V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);

?>
===EOF===
--EXPECT--
exception filter called.
string(5) "blarg"
===EOF===
