--TEST--
Test V8::executeString() : Test PHP object construction controlled by JavaScript (with ctor)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

class Greeter {
	protected $_name = null;

	function __construct($name) {
		echo "ctor called (php)\n";
		$this->_name = $name;
	}

    function sayHello() {
        echo "Hello ".$this->_name."\n";
    }   
}

$v8->greeter = new Greeter("John");

$v8->executeString('
    function JsGreeter(name) {
		print("ctor called (js)\n");
		this.name = name;
	};

    JsGreeter.prototype.sayHello = function() {
        print("Hello " + this.name + "\n");
    };

    jsGreeter = new JsGreeter("Paul");
    jsGreeter.sayHello();

    jsGreeterNg = new jsGreeter.constructor("George");
    jsGreeterNg.sayHello();

    // -----  now the same using v8Js  -----

    PHP.greeter.sayHello();

    var ngGreeter = new PHP.greeter.constructor("Ringo");
    ngGreeter.sayHello();
');
?>
===EOF===
--EXPECT--
ctor called (php)
ctor called (js)
Hello Paul
ctor called (js)
Hello George
Hello John
ctor called (php)
Hello Ringo
===EOF===

