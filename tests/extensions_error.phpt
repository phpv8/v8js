--TEST--
Test V8::registerExtension() : Register extension with errors
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$handlebarsJs = "var root = typeof global !== 'undefined' ? global : window, \$Handlebars = 'test';";
echo "-- registerExtension --\n";
V8Js::registerExtension('handlebars', $handlebarsJs, [], true);
echo "-- creating V8Js object --\n";
$v8 = new V8Js();
var_dump($v8);
?>
===EOF===
--EXPECTF--
-- registerExtension --
-- creating V8Js object --
Exception thrown during bootstrapping
Extension or internal compilation error%sin handlebars at line 1.
Error installing extension 'handlebars'.

Warning: V8Js::__construct(): Failed to create V8 context. Check that registered extensions do not have errors. in %s on line %d
NULL
===EOF===
