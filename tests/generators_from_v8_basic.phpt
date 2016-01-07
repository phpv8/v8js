--TEST--
Test V8::executeString() : Generators V8 -> PHP
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
--FILE--
<?php

$js = <<<EOJS
function* TheGen() {
  for(var i = 0; i < 4; i ++) {
    yield i;
  }
}

TheGen.theValue = 23;

EOJS;

$v8 = new V8Js();
$v8->executeString($js);

// just get the Generator itself -- it's just a normal JS function to us,
// i.e. V8Js should create a V8Function object.
$gen = $v8->executeString('(TheGen)');
var_dump($gen);

// now instantiate the Generator and pass that back -- should become a
// V8Generator object that implements the Iterator interface
$gen = $v8->executeString('(TheGen())');
var_dump($gen);
var_dump($gen instanceof Iterator);

?>
===EOF===
--EXPECTF--
object(V8Function)#%d (1) {
  ["theValue"]=>
  int(23)
}
object(V8Generator)#%d (0) {
}
bool(true)
===EOF===
