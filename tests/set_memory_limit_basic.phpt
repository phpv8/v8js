--TEST--
Test V8::setMemoryLimit() : Memory limit can be set on V8Js object
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (getenv("SKIP_SLOW_TESTS")) {
	die("skip slow test");
}
skip_if_using_valgrind();
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
$v8->setMemoryLimit(10000000);

try {
    var_dump($v8->executeString($JS, 'basic.js'));
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
