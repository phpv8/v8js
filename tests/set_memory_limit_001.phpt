--TEST--
Test V8::setMemoryLimit() : Memory limit applied to V8Function calls
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
var jsfunc = function() {
    var text = "abcdefghijklmnopqrstuvwyxz0123456789";
    var memory = "";
    for (var i = 0; i < 100; ++i) {
	for (var j = 0; j < 10000; ++j) {
            memory += text;
	}
	sleep(0);
    }
};
jsfunc;
EOT;

$v8 = new V8Js();
$v8->setMemoryLimit(10000000);

$func = $v8->executeString($JS);
var_dump($func);

try {
    $func();
} catch (V8JsMemoryLimitException $e) {
    print get_class($e); print PHP_EOL;
    print $e->getMessage(); print PHP_EOL;
}
?>
===EOF===
--EXPECTF--
object(V8Function)#%d (0) {
}
V8JsMemoryLimitException
Script memory limit of 10000000 bytes exceeded
===EOF===
