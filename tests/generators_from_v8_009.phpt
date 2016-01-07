--TEST--
Test V8::executeString() : Generators V8 -> PHP (fatal error)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js = <<<EOJS
function* TheGen() {
  yield 23;
  yield PHP.getValue();
}

TheGen();
EOJS;

$v8 = new V8Js();
$v8->getValue = function() {
    trigger_error("you're gonna fail now", E_USER_ERROR);
};
$gen = $v8->executeString($js);

foreach($gen as $a) {
    var_dump($a);
}

?>
===EOF===
--EXPECTF--
int(23)

Fatal error: you're gonna fail now in %s
