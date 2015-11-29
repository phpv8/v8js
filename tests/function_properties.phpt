--TEST--
Test V8::executeString() : Set property on function
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$JS = <<< EOT
(function(exports) {
  // begin module code
  exports.hello = function() { return 'hello'; };
  // end module code
  return exports;
})({})
EOT;

$exports = $v8->executeString($JS, 'basic.js');
$exports->hello->foo = "bar";
$v8->func = $exports->hello;

$v8->executeString('print(PHP.func.foo + "\n");');

?>
===EOF===
--EXPECT--
bar
===EOF===
