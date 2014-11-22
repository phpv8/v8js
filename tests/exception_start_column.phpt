--TEST--
Test V8::executeString() : Test getJsStartColumn on script exception
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

try {
	$v8->executeString("print(blar());");
}
catch(V8JsScriptException $e) {
	var_dump($e->getJsStartColumn());
}

?>
===EOF===
--EXPECT--
int(6)
===EOF===
