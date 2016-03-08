--TEST--
Test V8::setAverageObjectSize() : Average object size can be set on V8Js object
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();
$v8->setAverageObjectSize(32768);

// there's no API to query the currently announced external memory allocation,
// hence not much we can do here...

?>
===EOF===
--EXPECT--
===EOF===
