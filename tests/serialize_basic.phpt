--TEST--
Test serialize(V8Js) : __sleep and __wakeup throw
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

try {
    $stored = serialize($v8);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

$stored = 'O:4:"V8Js":0:{}';

try {
    $b = unserialize($stored);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

$stored = 'O:4:"V8Js":1:{s:3:"foo";i:23;}';

try {
    $b = unserialize($stored);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

?>
===EOF===
--EXPECT--
string(13) "V8JsException"
string(50) "You cannot serialize or unserialize V8Js instances"
string(13) "V8JsException"
string(50) "You cannot serialize or unserialize V8Js instances"
string(13) "V8JsException"
string(50) "You cannot serialize or unserialize V8Js instances"
===EOF===
