--TEST--
Test V8::executeString() : test context separation
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
print(PHP.foo);
EOT;

$a = new V8Js();
$a->foo = 'from first.js';

try {
	$a->executeString($JS, 'first.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

echo "\n";

$b = new V8Js();
$b->foo = 'from second.js';

try {
	$b->executeString($JS, 'second.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

echo "\n";
?>
===EOF===
--EXPECTF--
from first.js
from second.js
===EOF===
