--TEST--
Test V8Js::setModuleNormaliser : Custom normalisation #003
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("./test");
var bar = require("test");
EOT;

$v8 = new V8Js();

// Caching is done based on the identifiers passed back
// by the module normaliser.  If it returns the same id
// for multiple require calls, the module loader callback
// will be called only once (as the others are cached)
$v8->setModuleNormaliser(function($base, $module) {
    var_dump($base, $module);
    return [ "path/to", "test-foo" ];
});

$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    if($module != "path/to/test-foo") {
	throw new \Exception("module caching fails");
    }
    return 'exports.bar = 23;';
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
string(0) ""
string(6) "./test"
setModuleLoader called for path/to/test-foo
string(0) ""
string(4) "test"
===EOF===
