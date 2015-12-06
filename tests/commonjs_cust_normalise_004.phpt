--TEST--
Test V8Js::setModuleNormaliser : Custom normalisation #004
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("foo");
EOT;

$v8 = new V8Js();

// If a module includes another module, $base must be set to the
// path of the first module (on the second call)
$v8->setModuleNormaliser(function($base, $module) {
    var_dump($base, $module);
    return [ "path/to", $module ];
});

$v8->setModuleLoader(function($module) {
    print("setModuleLoader called for ".$module."\n");
    switch($module) {
	case "path/to/foo":
	    return "require('bar');";

	case "path/to/bar":
	    return 'exports.bar = 23;';
    }
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
string(0) ""
string(3) "foo"
setModuleLoader called for path/to/foo
string(7) "path/to"
string(3) "bar"
setModuleLoader called for path/to/bar
===EOF===
