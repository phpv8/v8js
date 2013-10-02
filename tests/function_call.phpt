--TEST--
Test V8::executeString() : Call passed-back function (directly)
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
echo $exports->hello()."\n";

?>
===EOF===
--EXPECT--
hello
===EOF===
