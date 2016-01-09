--TEST--
Test V8::executeString() : Generators PHP -> V8 (yield from)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

// Actually this check is a bit bad as it tests import, but currently
// there is no flag we can check for export
if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
--FILE--
<?php

function FibonacciGenerator()
{
    $i = 0;
    $j = 1;
    for(;;) {
        yield $j;
        list($i, $j) = array($j, $i + $j);
    }
}

$v8 = new V8Js();
$v8->fibs = FibonacciGenerator();

$JS = <<<EOJS
function* prefixer() {
    yield* arguments;
    yield* PHP.fibs;
}

var gen = prefixer(23, 42);

for(var i = 0; i < 10; i ++) {
    var_dump(gen.next().value);
}
EOJS;

$v8->executeString($JS);
?>
===EOF===
--EXPECT--
int(23)
int(42)
int(1)
int(1)
int(2)
int(3)
int(5)
int(8)
int(13)
int(21)
===EOF===
