--TEST--
Test V8::setTimeLimit() : Time limit can be prolonged
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var text = "abcdefghijklmnopqrstuvwyxz0123456789";
for (var j = 0; j < 100; ++j) {
    PHP.prolongTimeLimit();
    for (var i = 0; i < 3000; ++i) {
	var encoded = encodeURI(text);
    }
}
EOT;

$v8 = new V8Js();
$v8->setTimeLimit(25);

$v8->prolongTimeLimit = function() use ($v8) {
    $v8->setTimeLimit(25);
};

$v8->executeString($JS);
?>
===EOF===
--EXPECTF--
===EOF===
