--TEST--
Test V8::executeString() : Simple test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
len = print('Hello' + ' ' + 'World!' + "\\n");
len;
EOT;

$v8 = new V8Js();

try {
	var_dump($v8->executeString($JS, 'basic.js'));
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECT--
Hello World!
int(13)
===EOF===
