--TEST--
Test V8::executeString() : Dynamic closure call test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo
{
  public static function bar($s)
  {
     return 'Hello ' . $s;
  }
}

$a = new V8Js();
$b = array('Foo', 'bar');
$a->func = function ($arg) use ($b) { return call_user_func($b, $arg); };

try {
  $a->executeString('print(PHP.func + "\n"); print(PHP.func("world") + "\n");', "closure_test.js");
} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
?>
===EOF===
--EXPECT--
[object Closure]
Hello world
===EOF===
