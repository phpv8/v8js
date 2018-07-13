--TEST--
Test V8::executeString() : export of recursive array
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$a = [];
$a[] = &$a;
$a[] = 23;

$v8 = new V8Js();
$v8->foo = $a;
$v8->executeString('var_dump(PHP.foo);');

?>
===EOF===
--EXPECT--
array(2) {
  [0] =>
  NULL
  [1] =>
  int(23)
}
===EOF===
