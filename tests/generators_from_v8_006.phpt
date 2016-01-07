--TEST--
Test V8::executeString() : Generators V8 -> PHP (yield from)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
--FILE--
<?php

function PhpGen() {
    yield 23;

    $js = <<<EOJS
function* TheGen() {
  for(var i = 0; i < 4; i ++) {
    yield i;
  }
}

TheGen();
EOJS;

    $v8 = new V8Js();
    $jsGen = $v8->executeString($js);

    yield from $jsGen;
}

$gen = PhpGen();

foreach($gen as $a) {
    var_dump($a);
}

?>
===EOF===
--EXPECTF--
int(23)
int(0)
int(1)
int(2)
int(3)
===EOF===
