--TEST--
Test V8::executeString() : Export PHP properties on ArrayAccess objects
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    private $data = Array('one', 'two', 'three');

    private $privFoo = 23;
    protected $protFoo = 23;
    public $pubFoo = 42;

    /* We can have a length property on the PHP object, but the length property
     * of the JS object will still call count() method.  Anyways it should be
     * accessibly as $length. */
    public $length = 42;

    public function offsetExists($offset) {
	return isset($this->data[$offset]);
    }

    public function offsetGet($offset) {
	return $this->data[$offset];
    }

    public function offsetSet($offset, $value) {
	echo "set[$offset] = $value\n";
	$this->data[$offset] = $value;
    }

    public function offsetUnset($offset) {
        throw new Exception('Not implemented');
    }

    public function count() {
        return count($this->data);
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$v8->executeString('var_dump(PHP.myarr.privFoo);');
$v8->executeString('var_dump(PHP.myarr.protFoo);');
$v8->executeString('var_dump(PHP.myarr.pubFoo);');

/* This should call count(), i.e. return 3 */
$v8->executeString('var_dump(PHP.myarr.length);');

/* This should print the value of the $length property */
$v8->executeString('var_dump(PHP.myarr.$length);');

?>
===EOF===
--EXPECT--
NULL
NULL
int(42)
int(3)
int(42)
===EOF===
