--TEST--
Test V8Function::__call() : Check v8::TryCatch behaviour
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');
?>
--FILE--
<?php
$sandbox = new V8Js();

$cb = $sandbox->executeString('(function() { return oh; });');

try {
    $cb();
} catch(\Exception $e) {
    echo "caught: ".$e->getMessage()."\n";
}
?>
===EOF===
--EXPECT--
caught: V8Js::compileString():1: ReferenceError: oh is not defined
===EOF===
