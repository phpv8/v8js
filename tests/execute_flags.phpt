--TEST--
Test V8::executeString() : Forcing to arrays (return value conversion)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php 

$js = <<<'EOT'
var a = { 'hello' : 'world' }; a;
EOT;

$v8 = new V8Js();

try {
	var_dump($v8->executeString($js, 'assoc_no_flags.js'));
	echo "---\n";
	var_dump($v8->executeString($js, 'assoc_force_to_array.js', V8Js::FLAG_FORCE_ARRAY));
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
object(V8Object)#%d (1) {
  ["hello"]=>
  string(5) "world"
}
---
array(1) {
  ["hello"]=>
  string(5) "world"
}
===EOF===
