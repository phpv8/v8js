--TEST--
Test serialize(V8Function) : __sleep and __wakeup throw
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$obj = $v8->executeString('(function() { })');

var_dump($obj);

try {
    $stored = serialize($obj);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

$stored = 'O:10:"V8Function":0:{}';

try {
    $obj2 = unserialize($stored);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

var_dump(isset($obj2));

$stored = 'O:10:"V8Function":1:{s:3:"foo";i:23;}';

try {
    $obj = unserialize($stored);
}
catch(\V8JsException $e) {
    var_dump(get_class($e));
    var_dump($e->getMessage());
}

var_dump(isset($obj3));

?>
===EOF===
--EXPECT--
object(V8Function)#2 (0) {
}
string(13) "V8JsException"
string(56) "You cannot serialize or unserialize V8Function instances"
string(13) "V8JsException"
string(56) "You cannot serialize or unserialize V8Function instances"
bool(false)
string(13) "V8JsException"
string(56) "You cannot serialize or unserialize V8Function instances"
bool(false)
===EOF===
