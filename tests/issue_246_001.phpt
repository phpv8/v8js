--TEST--
Test V8::executeString() : Handle Z_TYPE == IS_REFERENCE (issue #246)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');
?>
--FILE--
<?php
$v8 = new V8Js();
$array = ['lorem' => 'ipsum'];
array_walk_recursive($array, function (&$item) {});
$v8->some_array = $array;
$v8->executeString('var_dump(PHP.some_array.lorem);');
?>
===EOF===
--EXPECT--
string(5) "ipsum"
===EOF===
