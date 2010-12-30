--TEST--
Test V8::executeString() : PHP variables via get accessor
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
display_errors=off
--FILE--
<?php

$JS = <<<'EOT'
print(typeof $foobar + "\n"); // Undefined
print(myobj.$foobar + "\n");  // Undefined (in 1st run!)
print(myobj.$_SERVER['REQUEST_TIME'] + "\n");
myobj.$foobar = 'CHANGED'; // should be read only!
print(myobj.$foobar + "\n");  // Undefined (in 1st run!)
EOT;

$a = new V8Js("myobj", array('$_SERVER' => '_SERVER', '$foobar' => 'myfoobar'));
$a->executeString($JS, "test1.js");

$myfoobar = 'myfoobarfromphp';

$a->executeString($JS, "test2.js");

// Check that variables do not get in object .. 
var_dump($a->myfoobar, $a->foobar);

?>
===EOF===
--EXPECTF--
undefined
undefined
%d
undefined
undefined
myfoobarfromphp
%d
myfoobarfromphp
NULL
NULL
===EOF===
