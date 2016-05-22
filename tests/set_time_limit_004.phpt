--TEST--
Test V8::setTimeLimit() : Time limit can be prolonged
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
/* Spend 75 * >10ms in the loop, i.e. at least 750ms; hence
 * it should be killed if prolonging doesn't work. */
for (var j = 0; j < 30; ++j) {
    PHP.prolongTimeLimit();
    var start = (new Date()).getTime();
    var encoded = encodeURI(text);

    while ((new Date()).getTime() - start < 10) {
        /* pass about 10ms in the loop, then prolong */
        var encoded = encodeURI(text);
    }
}
EOT;

$v8 = new V8Js();
$v8->setTimeLimit(300);

$v8->prolongTimeLimit = function() use ($v8) {
    $v8->setTimeLimit(300);
};

$v8->executeString($JS);
?>
===EOF===
--EXPECTF--
===EOF===
