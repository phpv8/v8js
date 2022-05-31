--TEST--
Test V8::setExceptionProxyFactory() : Proxy handling on exception in setModuleNormaliser
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

$v8->setExceptionProxyFactory(function (Throwable $ex) {
	echo "exception proxy factory called.\n";
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
exception proxy factory called.
string(5) "blarg"
===EOF===
