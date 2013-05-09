--TEST--
Test V8::executeString() : Calling construct twice
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
print('Hello' + ' ' + 'World!' + "\\n");
EOT;

$v8 = new V8Js("myObj");
$v8->__construct();

try {
	$v8->executeString($JS, 'basic.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECT--
Hello World!
===EOF===
