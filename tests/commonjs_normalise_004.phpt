--TEST--
Test V8Js::setModuleLoader : Path normalisation #004
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("foo/test");
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");

    switch($module) {
    case 'foo/test':
        return 'require("./blar");';
    case 'foo/blar':
        return 'exports.bar = 23;';
    }
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
setModuleLoader called for foo/test
setModuleLoader called for foo/blar
===EOF===
