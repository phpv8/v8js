--TEST--
Test V8::executeString() : Generators PHP -> V8 (instantiate in JS, iterate in PHP)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$v8->Gen = function() {
    for($i = 0; $i < 4; $i ++) {
        yield $i;
    }
};

$gen = $v8->executeString('(PHP.Gen())');

foreach($gen as $i) {
    var_dump($i);
}

?>
===EOF===
--EXPECTF--
int(0)
int(1)
int(2)
int(3)
===EOF===
