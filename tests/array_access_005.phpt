--TEST--
Test V8::executeString() : Export __invoke method on ArrayAccess objects
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    private $data = Array('one', 'two', 'three');

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

    public function __invoke() {
	echo "__invoke called!\n";
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$v8->executeString('PHP.myarr();');

$v8->executeString('var_dump(PHP.myarr.length);');
$v8->executeString('var_dump(PHP.myarr.join(", "));');

?>
===EOF===
--EXPECT--
__invoke called!
int(3)
string(15) "one, two, three"
===EOF===
