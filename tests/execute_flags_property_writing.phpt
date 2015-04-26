--TEST--
Test V8::executeString() : Forcing to arrays (property writing)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = <<<'EOT'
PHP.test.foo = { "hello": "world" };
EOT;

$v8 = new V8Js();
$v8->test = new stdClass();

try {
	$v8->executeString($js, 'no_flags.js');
	var_dump($v8->test);
	echo "---\n";
	$v8->executeString($js, 'force_to_array.js', V8Js::FLAG_FORCE_ARRAY);
	var_dump($v8->test);
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
object(stdClass)#%d (1) {
  ["foo"]=>
  object(V8Object)#%d (1) {
    ["hello"]=>
    string(5) "world"
  }
}
---
object(stdClass)#%d (1) {
  ["foo"]=>
  array(1) {
    ["hello"]=>
    string(5) "world"
  }
}
===EOF===
