--TEST--
Test V8::registerExtension() : Basic extension registering
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

V8Js::registerExtension('a', 'print("world!\n");', array('b'));
V8Js::registerExtension('b', 'print("Hello ");');

var_dump(V8JS::getExtensions());

$a = new V8Js('myobj', array(), array('a'));
?>
===EOF===
--EXPECT--
array(2) {
  ["a"]=>
  array(2) {
    ["auto_enable"]=>
    bool(false)
    ["deps"]=>
    array(1) {
      [0]=>
      string(1) "b"
    }
  }
  ["b"]=>
  array(1) {
    ["auto_enable"]=>
    bool(false)
  }
}
Hello world!
===EOF===
