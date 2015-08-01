--TEST--
Test V8Js::setModuleLoader : module cache seperated per isolate
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("test");
var baz = require("test");
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    return 'exports.bar = 23;';
});

$v8two = new V8Js();
$v8two->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    return 'exports.bar = 23;';
});

$v8->executeString($JS, 'module.js');
echo "--- v8two ---\n";
$v8two->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
setModuleLoader called for test
--- v8two ---
setModuleLoader called for test
===EOF===
