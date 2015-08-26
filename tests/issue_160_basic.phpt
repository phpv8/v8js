--TEST--
Test V8::executeString() : Issue #160 V8Function affected by V8Js::FLAG_FORCE_ARRAY
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

$JS = <<<EOT
(function(foo) { print(foo); });
EOT;

$func = $v8->executeString($JS, 'test', V8Js::FLAG_FORCE_ARRAY);

var_dump($func);
$func("Test-Foo Func Call\n");
?>
===EOF===
--EXPECTF--
object(V8Function)#%d (0) {
}
Test-Foo Func Call
===EOF===
