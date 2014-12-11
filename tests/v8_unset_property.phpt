--TEST--
Test V8::executeString() : unset property on V8Object
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$a = $v8->executeString('var a = { bla: 23 }; a');
var_dump($a);

// properties on $a should be unset'able
unset($a->bla);
var_dump($a);

$v8->executeString('print(a.bla + "\\n");');

unset($v8);

try {
	// unset not valid, if $v8 object is disposed
	unset($a->bla);
}
catch(V8JsException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECTF--
object(V8Object)#%d (1) {
  ["bla"]=>
  int(23)
}
object(V8Object)#%d (0) {
}
undefined
string(55) "Can't access V8Object after V8Js instance is destroyed!"
===EOF===
