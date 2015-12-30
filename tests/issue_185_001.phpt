--TEST--
Test V8::executeString() : Issue #185 this on direct invocation of method
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$JS = <<<EOT

function Bar(i) {
    this.theValue = i;
}
Bar.prototype.tell = function() {
    var_dump(this.theValue);
    var_dump(typeof this.exit);
};
var inst = new Bar(23);
var fn = inst.tell;
fn();
EOT;

$v8->executeString($JS);

// now fetch `inst` from V8 and call method from PHP
$fn = $v8->executeString('(inst.tell)');
$fn();
?>
===EOF===
--EXPECT--
NULL
string(8) "function"
NULL
string(8) "function"
===EOF===
