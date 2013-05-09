--TEST--
Test V8::executeString() : test context preserving
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS_set = <<< EOT
var preserved = "ORIGINAL";
print("Set variable (" + PHP.ctx + ")\\n");
EOT;

$JS_change = <<< EOT
preserved = "CHANGED";
print("Change variable (" + PHP.ctx + ")\\n");
EOT;

$JS_read = <<< EOT
print("Read variable (" + PHP.ctx + ")\\n");
print("Result: " + preserved + "\\n");
EOT;

// First context: Set variable
$a = new V8Js();
$a->ctx = '#1';

try {
	echo '1. ';
	$a->executeString($JS_set, 'set.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

// Second context: Change variable
$b = new V8Js();
$b->ctx = '#2';

try {
	echo '2. ';
	$b->executeString($JS_change, 'change.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

// First context: Read variable
try {
	echo '3. ';
	$a->executeString($JS_read, 'read.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

// First context: Change variable
try {
	echo '4. ';
	$a->executeString($JS_change, 'change.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}

// First context: Read variable again
try {
	echo '5. ';
	$a->executeString($JS_read, 'read.js');
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECT--
1. Set variable (#1)
2. Change variable (#2)
3. Read variable (#1)
Result: ORIGINAL
4. Change variable (#1)
5. Read variable (#1)
Result: CHANGED
===EOF===
