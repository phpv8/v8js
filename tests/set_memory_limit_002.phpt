--TEST--
Test V8::setMemoryLimit() : Memory limit can be set but does not trigger when not exceeded
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
    var text = "abcdefghijklmnopqrstuvwyxz0123456789"; // 36 bytes
    var memory = "";
    // should generate ~ 800 kB
    for (var i = 0; i < 22; ++i) {
        for (var j = 0; j < 1000; ++j) {
            memory += text;
        }
        sleep(0);
    }

    return memory;
};
jsfunc;
EOT;

$v8 = new V8Js();
$v8->setMemoryLimit(10_000_000);

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
===EOF===
