--TEST--
Test V8::executeString() : Generators V8 -> PHP (direct)
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

TheGen();
EOJS;

$v8 = new V8Js();
$gen = $v8->executeString($js);

var_dump($gen->current());

// JS generators don't have the key concept (-> just "false")
var_dump($gen->key());

// fetching multiple times shouldn't leak
var_dump($gen->current());
var_dump($gen->current());

$gen->next(); // 1
var_dump($gen->current());

$gen->next(); // 2
var_dump($gen->current());

$gen->next(); // 3
var_dump($gen->current());
var_dump($gen->valid());

$gen->next(); // undef
var_dump($gen->current());
var_dump($gen->valid());

?>
===EOF===
--EXPECTF--
int(0)
bool(false)
int(0)
int(0)
int(1)
int(2)
int(3)
bool(true)
NULL
bool(false)
===EOF===
