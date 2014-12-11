--TEST--
Test serialize(V8Object) : __sleep and __wakeup throw
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$obj = $v8->executeString('({ foo: 23 })');

var_dump($obj);

try {
    $stored = serialize($obj);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

$stored = 'O:8:"V8Object":0:{}';

try {
    $obj = unserialize($stored);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

$v8->foo = $obj;


?>
===EOF===
--EXPECT--
object(V8Object)#2 (1) {
  ["foo"]=>
  int(23)
}
string(13) "V8JsException"
string(54) "You cannot serialize or unserialize V8Object instances"
string(13) "V8JsException"
string(54) "You cannot serialize or unserialize V8Object instances"
===EOF===
