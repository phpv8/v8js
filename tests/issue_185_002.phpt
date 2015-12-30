--TEST--
Test V8::executeString() : Issue #185 this on function invocation
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$JS = <<<EOT

function fn() {
    var_dump(typeof this.exit);
};
fn();
EOT;

$v8->executeString($JS);

// now fetch `inst` from V8 and call method from PHP
$fn = $v8->executeString('(fn)');
$fn();
?>
===EOF===
--EXPECT--
string(8) "function"
string(8) "function"
===EOF===
