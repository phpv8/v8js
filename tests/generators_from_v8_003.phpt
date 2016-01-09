--TEST--
Test V8::executeString() : Generators V8 -> PHP (rewind)
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
  for(var i = 0; i < 4; i ++) {
    yield i;
  }
}

TheGen();
EOJS;

$v8 = new V8Js();
$gen = $v8->executeString($js);

foreach($gen as $a) {
    var_dump($a);
}

foreach($gen as $a) {
    var_dump($a);
}

?>
===EOF===
--EXPECTF--
int(0)
int(1)
int(2)
int(3)

Fatal error: Uncaught V8JsException: V8Generator::rewind not supported by ES6 in %s
Stack trace:
#0 %s: V8Generator->rewind()
#1 {main}
  thrown in %s on line 20
