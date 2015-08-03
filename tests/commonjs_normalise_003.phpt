--TEST--
Test V8Js::setModuleLoader : Path normalisation #003
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("foo/test");
var foo = require("foo/bar/baz/test");
var foo = require("foo//bar//baz//blub");
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
setModuleLoader called for foo/test
setModuleLoader called for foo/bar/baz/test
setModuleLoader called for foo/bar/baz/blub
===EOF===
