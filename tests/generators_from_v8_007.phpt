--TEST--
Test V8::executeString() : Generators V8 -> PHP (throw JS)
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
  throw new Error('blar');
}

TheGen();
EOJS;

$v8 = new V8Js();
$gen = $v8->executeString($js);

foreach($gen as $a) {
    var_dump($a);
}

?>
===EOF===
--EXPECTF--
int(23)

Fatal error: Uncaught V8JsScriptException: %s Error: blar in %s
Stack trace:
#0 %s: V8Generator->next()
#1 {main}
  thrown in %s
