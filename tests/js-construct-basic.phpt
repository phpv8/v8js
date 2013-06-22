--TEST--
Test V8::executeString() : Test PHP object construction controlled by JavaScript (simple)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

class Greeter {
    function sayHello($a) {
        echo "Hello $a\n";
    }   
}

$v8->greeter = new Greeter();
$v8->executeString('
    function JsGreeter() { };
    JsGreeter.prototype.sayHello = function(a) {
        print("Hello " + a + "\n");
    };

    jsGreeter = new JsGreeter();
    jsGreeter.sayHello("Paul");

    jsGreeterNg = new jsGreeter.constructor();
    jsGreeterNg.sayHello("George");

    // -----  now the same using v8Js  -----

    PHP.greeter.sayHello("John");

    var ngGreeter = new PHP.greeter.constructor();
    ngGreeter.sayHello("Ringo");
');
?>
===EOF===
--EXPECT--
Hello Paul
Hello George
Hello John
Hello Ringo
===EOF===
