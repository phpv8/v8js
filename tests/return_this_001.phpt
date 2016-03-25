--TEST--
Test V8::executeString() : return this (aka fluent setters, JS-side)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = <<<EOJS
function Bar() {
}

Bar.prototype.setFoo = function(value) {
	this.foo = value;
	return this;
}

Bar.prototype.setBar = function(value) {
	this.bar = value;
	return this;
}

theBar = new Bar();
(theBar);
EOJS;

$v8 = new V8Js();
$bar = $v8->executeString($js);

$ret = $bar->setFoo(23)->setBar(42);
var_dump($bar === $ret);

$v8->executeString('var_dump(theBar);');

?>
===EOF===
--EXPECTF--
bool(true)
object(Bar)#%d (2) {
  ["foo"] =>
  int(23)
  ["bar"] =>
  int(42)
}
===EOF===
