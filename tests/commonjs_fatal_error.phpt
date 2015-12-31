--TEST--
Test V8Js::setModuleLoader : Handle fatal errors gracefully
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

$v8->setModuleLoader(function() {
    trigger_error('some fatal error', E_USER_ERROR);
});

$v8->executeString(' require("foo"); ');
?>
===EOF===
--EXPECTF--
Fatal error: some fatal error in %s%ecommonjs_fatal_error.php on line 5
