--TEST--
Test V8::executeString() : Issue #185 Wrong this on V8Object method invocation
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
};
var inst = new Bar(23);
inst.tell();
EOT;

$v8->executeString($JS);

// now fetch `inst` from V8 and call method from PHP
$inst = $v8->executeString('(inst)');
$inst->tell();
?>
===EOF===
--EXPECT--
int(23)
int(23)
===EOF===
