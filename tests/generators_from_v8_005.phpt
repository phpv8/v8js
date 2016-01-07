--TEST--
Test V8::executeString() : Generators V8 -> PHP (instantiate in PHP + iterate in JS)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = <<<EOJS
function* TheGen() {
  for(var i = 0; i < 4; i ++) {
    yield i;
  }
}

TheGen;
EOJS;

$v8 = new V8Js();
$TheGen = $v8->executeString($js);
$gen = $TheGen();

$js = <<<EOJS
(function(gen) {
  for(var i of gen) {
    var_dump(i);
  }
})
EOJS;
$fn = $v8->executeString($js);
$fn($gen);

?>
===EOF===
--EXPECTF--
int(0)
int(1)
int(2)
int(3)
===EOF===
