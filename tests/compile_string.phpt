--TEST--
Test V8::compileString() : Compile and run a script
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php 

$js = <<<'EOT'
var a = { 'hello' : 'world' }; a;
EOT;

$js2 = <<<'EOT'
var a = { 'foo' : 'bar' }; a;
EOT;

$v8 = new V8Js();

try {
  $script_a = $v8->compileString($js, 'a.js');
	var_dump(is_resource($script_a));
  $script_b = $v8->compileString($js2, 'b.js');
	var_dump(is_resource($script_b));
	var_dump($v8->executeScript($script_a));
	var_dump($v8->executeScript($script_b));
	var_dump($v8->executeScript($script_a));
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECT--
bool(true)
bool(true)
object(V8Object)#2 (1) {
  ["hello"]=>
  string(5) "world"
}
object(V8Object)#2 (1) {
  ["foo"]=>
  string(3) "bar"
}
object(V8Object)#2 (1) {
  ["hello"]=>
  string(5) "world"
}
===EOF===
