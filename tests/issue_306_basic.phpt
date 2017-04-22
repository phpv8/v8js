--TEST--
Test V8::executeString() : Issue #306 V8 crashing on toLocaleString()
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$expr = 'new Date("10/11/2009").toLocaleString("en-us", { month: "long" });';
$result = $v8->executeString($expr);

// V8 can be compiled with i18n support and without;
// without i18n support however toLocaleString doesn't really work,
// it just returns the date string...

if ($result === 'October') {
    var_dump(true);
} else {
    $expr = 'new Date("10/11/2009").toString();';
    var_dump($v8->executeString($expr) === $result);
}

?>
===EOF===
--EXPECT--
bool(true)
===EOF===