--TEST--
Test V8::executeString() : direct construction is prohibited
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

# these are not allowed
echo "-- NOT ALLOWED --\n";
try {
	$a = new V8Object;
} catch (V8JsException $e) {
	var_dump($e->getMessage());
}
try {
	$a = new V8Function;
} catch (V8JsException $e) {
	var_dump($e->getMessage());
}

# but these are allowed
echo "-- ALLOWED --\n";
$v8 = new V8Js();
$o = $v8->executeString("({foo:1})");
var_dump($o);
$f = $v8->executeString("(function() { return 1; })");
var_dump($f);

# but these are not allowed
echo "-- NOT ALLOWED --\n";
try {
	$oo = new $o();
} catch (V8JsException $e) {
	var_dump($e->getMessage());
}
try {
	$ff = new $f;
} catch (V8JsException $e) {
	var_dump($e->getMessage());
}

// free memory
$o = null; $f = null; $v8 = null;
?>
===EOF===
--EXPECTF--
-- NOT ALLOWED --
string(36) "Can't directly construct V8 objects!"
string(36) "Can't directly construct V8 objects!"
-- ALLOWED --
object(V8Object)#%d (1) {
  ["foo"]=>
  int(1)
}
object(V8Function)#%d (0) {
}
-- NOT ALLOWED --
string(36) "Can't directly construct V8 objects!"
string(36) "Can't directly construct V8 objects!"
===EOF===
