--TEST--
Test V8::executeString() : Time limit
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (getenv("SKIP_SLOW_TESTS")) {
	die("skip slow test");
}
?>
--FILE--
<?php

$JS = <<< EOT
var text = "abcdefghijklmnopqrstuvwyxz0123456789";
for (var i = 0; i < 10000000; ++i) {
    var encoded = encodeURI(text);
}
EOT;

$v8 = new V8Js();

try {
    var_dump($v8->executeString($JS, 'basic.js', V8Js::FLAG_NONE, 1000));
} catch (V8JsTimeLimitException $e) {
    print get_class($e); print PHP_EOL;
    print $e->getMessage(); print PHP_EOL;
}
?>
===EOF===
--EXPECT--
V8JsTimeLimitException
Script time limit of 1000 milliseconds exceeded
===EOF===
