--TEST--
Test V8::executeString() : Generators V8 -> PHP (throw PHP)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
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
    throw new \Exception('this shall not work');
};
$gen = $v8->executeString($js);

foreach($gen as $a) {
    var_dump($a);
}

?>
===EOF===
--EXPECTF--
int(23)

Fatal error: Uncaught Exception: this shall not work in %s
Stack trace:
#0 [internal function]: {closure}()
#1 [internal function]: Closure->__invoke()
#2 %s: V8Generator->next()
#3 {main}
  thrown in %s
