--TEST--
Test V8::executeString() : Exception propagation test 2
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
	private $v8 = NULL;

	public function __construct()
	{
		$this->v8 = new V8Js(null, array(), array(), false);
		$this->v8->foo = $this;
		$this->v8->executeString('fooobar', 'throw_0');
		var_dump($this->v8->getPendingException());
		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " caught!\n"); }', 'trycatch1');
		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " caught!\n"); }', 'trycatch2');
	}

	public function bar()
	{
		echo "To Bar!\n";
		$this->v8->executeString('throw new Error();', 'throw_1');
	}
}
    
try {
	$foo = new Foo();
} catch (V8JsScriptException $e) {
	echo "PHP Exception: ", $e->getMessage(), "\n"; //var_dump($e);
}
?>
===EOF===
--EXPECTF--
object(V8JsScriptException)#%d (13) {
  ["message":protected]=>
  string(49) "throw_0:1: ReferenceError: fooobar is not defined"
  ["string":"Exception":private]=>
  string(0) ""
  ["code":protected]=>
  int(0)
  ["file":protected]=>
  string(%d) "%s"
  ["line":protected]=>
  int(10)
  ["trace":"Exception":private]=>
  array(2) {
    [0]=>
    array(6) {
      ["file"]=>
      string(%d) "%s"
      ["line"]=>
      int(10)
      ["function"]=>
      string(13) "executeString"
      ["class"]=>
      string(4) "V8Js"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(2) {
        [0]=>
        string(7) "fooobar"
        [1]=>
        string(7) "throw_0"
      }
    }
    [1]=>
    array(6) {
      ["file"]=>
      string(%d) "%s"
      ["line"]=>
      int(24)
      ["function"]=>
      string(11) "__construct"
      ["class"]=>
      string(3) "Foo"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(0) {
      }
    }
  }
  ["previous":"Exception":private]=>
  NULL
  ["JsFileName":protected]=>
  string(7) "throw_0"
  ["JsLineNumber":protected]=>
  int(1)
  ["JsStartColumn":protected]=>
  int(0)
  ["JsEndColumn":protected]=>
  int(1)
  ["JsSourceLine":protected]=>
  string(7) "fooobar"
  ["JsTrace":protected]=>
  string(57) "ReferenceError: fooobar is not defined
    at throw_0:1:1"
}
To Bar!
Error caught!
PHP Exception: throw_0:1: ReferenceError: fooobar is not defined
===EOF===
