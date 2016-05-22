--TEST--
Test V8::setTimeLimit() : Time limit can be imposed later on
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
    PHP.imposeTimeLimit();
    var text = "abcdefghijklmnopqrstuvwyxz0123456789";
    for (var i = 0; i < 10000000; ++i) {
	var encoded = encodeURI(text);
    }
};
jsfunc;
EOT;

$v8 = new V8Js();
$v8->imposeTimeLimit = function() use ($v8) {
    $v8->setTimeLimit(100);
};

$func = $v8->executeString($JS);
var_dump($func);

try {
    $func();
} catch (V8JsTimeLimitException $e) {
    print get_class($e); print PHP_EOL;
    print $e->getMessage(); print PHP_EOL;
}
?>
===EOF===
--EXPECTF--
object(V8Function)#%d (0) {
}
V8JsTimeLimitException
Script time limit of 100 milliseconds exceeded
===EOF===
