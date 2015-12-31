--TEST--
Test V8Js::setModuleNormaliser : Custom normalisation #001
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("./test");
EOT;

$v8 = new V8Js();

$v8->setModuleNormaliser(function($base, $module) {
    var_dump($base, $module);
    return [ "", "test" ];
});

$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    return 'exports.bar = 23;';
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
string(0) ""
string(6) "./test"
setModuleLoader called for test
===EOF===
