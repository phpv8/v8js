--TEST--
Test V8::compileString() : Check compiled script isolate processing
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
$v8two = new V8Js();

try {
  $script_a = $v8->compileString($js, 'a.js');
	var_dump($script_a);
  $script_b = $v8two->compileString($js2, 'b.js');
	var_dump($script_b);
	var_dump($v8->executeScript($script_a));
	var_dump($v8->executeScript($script_b));
	var_dump($v8->executeScript($script_a));
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
resource(%d) of type (V8Js script)
resource(%d) of type (V8Js script)
object(V8Object)#%d (1) {
  ["hello"]=>
  string(5) "world"
}

Warning: Script resource from wrong V8Js object passed in %s on line %d
bool(false)
object(V8Object)#%d (1) {
  ["hello"]=>
  string(5) "world"
}
===EOF===
