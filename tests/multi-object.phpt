--TEST--
Test V8::executeString() : Use multiple V8js instances with objects
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class TestClass {
	protected $_instNo;

	public function __construct($num) {
		$this->_instNo = $num;
	}

	public function sayHello() {
		echo 'Hello World!  This is instance '.$this->_instNo."\n";
	}
}

$instances = array();
for($i = 0; $i < 5; $i ++) {
	$v8 = new V8Js();
	$v8->test = new TestClass($i);
	$instances[] = $v8;
}

$JS = <<< EOT
PHP.test.sayHello();
EOT;

foreach($instances as $v8) {
	$v8->executeString($JS, 'basic.js');
}

?>
===EOF===
--EXPECT--
Hello World!  This is instance 0
Hello World!  This is instance 1
Hello World!  This is instance 2
Hello World!  This is instance 3
Hello World!  This is instance 4
===EOF===
