--TEST--
Test V8Js::setModuleLoader : Returned modules are cached
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("test");
var bar = require("test2");
var baz = require("test");
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    return 'exports.bar = 23;';
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
setModuleLoader called for test
setModuleLoader called for test2
===EOF===
