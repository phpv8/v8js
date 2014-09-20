--TEST--
Test V8Js::setModuleLoader : Assign result multiple times
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var foo = require("test");
var bar = require("test");
var_dump(foo.bar);
var_dump(bar.bar);
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
  return 'exports.bar = 23;';
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
int(23)
int(23)
===EOF===
