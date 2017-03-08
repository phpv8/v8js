--TEST--
Test V8::executeString() : Check long 64-bit export from PHP to JS
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');
if (4 == PHP_INT_SIZE)
	die("SKIP test not supported on 32bit PHP");
?>
--FILE--
<?php

$addInt = function ($a, $b) {
    return $a + $b;
};

$v8 = new V8Js('PHP');
$v8->add = $addInt;
$v8->p1 = 10;
$v8->p2 = pow(2,45);

var_dump($v8->p2);

var_dump($v8->executeString('
	var res = PHP.p1 + PHP.p2;
	"p1:" + PHP.p1 +
	", p2:" + PHP.p2 +
	", PHP.add(p1,p2)=" + PHP.add(PHP.p1, PHP.p2) +
	", p1+p2=" + res +
	" -> " + (new Date(res)).toISOString();
	'));
?>
===EOF===
--EXPECT--
int(35184372088832)
string(105) "p1:10, p2:35184372088832, PHP.add(p1,p2)=35184372088842, p1+p2=35184372088842 -> 3084-12-12T12:41:28.842Z"
===EOF===
