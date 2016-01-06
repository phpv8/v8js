--TEST--
Test V8::executeString() : Test PHP object construction controlled by JavaScript (protected ctor)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v8 = new V8Js();

class Greeter {
	protected $_name = null;

	protected function __construct($name) {
		echo "ctor called (php)\n";
		$this->_name = $name;
	}

	static function getInstance($name) {
		return new Greeter($name);
	}

	function sayHello() {
		echo "Hello ".$this->_name."\n";
	}
}

$v8->greeter = Greeter::getInstance("John");

try {
	$v8->executeString('
		PHP.greeter.sayHello();

		var ngGreeter = new PHP.greeter.constructor("Ringo");
		ngGreeter.sayHello();
	', 'ctor-test');
} catch(V8JsScriptException $e) {
	echo "caught js exception\n";
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
ctor called (php)
Hello John
caught js exception
object(V8JsScriptException)#%d (13) {
  ["message":protected]=>
  string(56) "ctor-test:4: Call to protected __construct() not allowed"
  ["string":"Exception":private]=>
  string(0) ""
  ["code":protected]=>
  int(0)
  ["file":protected]=>
  string(%d) "%s"
  ["line":protected]=>
  int(29)
  ["trace":"Exception":private]=>
  array(1) {
    [0]=>
    array(6) {
      ["file"]=>
      string(%d) "%s"
      ["line"]=>
      int(29)
      ["function"]=>
      string(13) "executeString"
      ["class"]=>
      string(4) "V8Js"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(2) {
        [0]=>
        string(%d) "
		PHP.greeter.sayHello();

		var ngGreeter = new PHP.greeter.constructor("Ringo");
		ngGreeter.sayHello();
	"
        [1]=>
        string(9) "ctor-test"
      }
    }
  }
  ["previous":"Exception":private]=>
  NULL
  ["JsFileName":protected]=>
  string(9) "ctor-test"
  ["JsLineNumber":protected]=>
  int(4)
  ["JsStartColumn":protected]=>
  int(18)
  ["JsEndColumn":protected]=>
  int(19)
  ["JsSourceLine":protected]=>
  string(55) "		var ngGreeter = new PHP.greeter.constructor("Ringo");"
  ["JsTrace":protected]=>
  NULL
}
===EOF===

