--TEST--
Test V8::executeString() : Memory limit
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
var memory = "";
for (var i = 0; i < 100; ++i) {
    for (var j = 0; j < 10000; ++j) {
        memory += text;
    }
    sleep(0);
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