--TEST--
Test V8::executeString() : Method access from multiple derived classes
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class BaseClass {
        public function bla() {
            printf('print bla, called class: %s' . PHP_EOL, get_called_class());
        }
}

class Foo extends BaseClass {}

class Bar extends BaseClass {}

$v8 = new V8Js('PHP');
$v8->Foo = new Foo();
$v8->Bar = new Bar();

$code = <<<EOT
var_dump(PHP.Foo);
PHP.Foo.bla();
var_dump(PHP.Bar);
PHP.Bar.bla();
EOT;

$v8->executeString($code);

?>
===EOF===
--EXPECTF--
object(Foo)#%d (1) {
  ["bla"] =>
  object(Closure)#%d {
      function () { [native code] }
  }
}
print bla, called class: Foo
object(Bar)#%d (1) {
  ["bla"] =>
  object(Closure)#%d {
      function () { [native code] }
  }
}
print bla, called class: Bar
===EOF===

