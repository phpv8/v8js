--TEST--
Test V8::executeString() : Issue #116 V8Function injection into other V8Js
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$a = new V8Js();
$b = new V8Js();

$a->name = 'A';
$b->name = 'B';

$a->b = $b;
$a->executeString('PHP.b.test = function() { print("Hallo from within " + PHP.name + ".\\n"); };');

// in PHP we see the property
var_dump($b->test);

// we see (and can call) the function object in instance A
print("in A:\n");
$a->executeString('PHP.b.test();');

// in B the function object is not available
print("in B:\n");
$b->executeString('print(typeof PHP.b + "\\n");');

try {
	$b->executeString('PHP.test();');
}
catch(Exception $e) {
	var_dump($e->getMessage());
}

unset($a);
unset($b);
?>
===EOF===
--EXPECTF--
Warning: V8Js::executeString(): V8Function object passed to wrong V8Js instance in %s on line %d
object(V8Function)#%d (0) {
}
in A:
Hallo from within A.
in B:
undefined
string(%d) "V8Js::compileString():1: TypeError: %s is not a function"
===EOF===
