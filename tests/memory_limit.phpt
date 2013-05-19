--TEST--
Test V8::executeString() : Time limit
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$JS = <<< EOT
var text = "abcdefghijklmnopqrstuvwyxz0123456789";
var memory = "";
for (var i = 0; i < 1000000; ++i) {
    memory += text;
}
EOT;

$v8 = new V8Js();

try {
    var_dump($v8->executeString($JS, 'basic.js', V8Js::FLAG_NONE, 0, 10000000));
} catch (V8JsMemoryLimitException $e) {
    print get_class($e); print PHP_EOL;
    print $e->getMessage(); print PHP_EOL;
}
?>
===EOF===
--EXPECT--
V8JsMemoryLimitException
Script memory limit of 10000000 bytes exceeded
===EOF===