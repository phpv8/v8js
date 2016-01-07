--TEST--
Test V8::executeString() : Generators V8 -> PHP (properties)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
--FILE--
<?php

$js = <<<EOJS
function* TheGen() {
  yield 23;
  yield PHP.getValue();
}

var gen = TheGen();
gen.foo = 23;
gen.bar = function() { var_dump("Hello World"); };

gen;
EOJS;

$v8 = new V8Js();
$gen = $v8->executeString($js);

var_dump($gen->foo);
$gen->bar();

?>
===EOF===
--EXPECT--
int(23)
string(11) "Hello World"
===EOF===
