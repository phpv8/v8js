--TEST--
Test V8::executeString() : Test getJsStartColumn on script exception
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

// V8 started to return different start column numbers,
// hence let's do two errors and just look at the offset

try {
	$v8->executeString("print(blar());");
}
catch(V8JsScriptException $a) { }

try {
	$v8->executeString("(null); print(blar());");
}
catch(V8JsScriptException $b) { }

var_dump($b->getJsStartColumn() - $a->getJsStartColumn());

?>
===EOF===
--EXPECT--
int(8)
===EOF===
