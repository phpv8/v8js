--TEST--
Test V8::executeString() : in array (isset) behaviour of ArrayAccess
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    private $data = Array('one', null, 'three');

    public function offsetExists($offset) {
	return isset($this->data[$offset]);
    }

    public function offsetGet($offset) {
	return $this->data[$offset];
    }

    public function offsetSet($offset, $value) {
	$this->data[$offset] = $value;
    }

    public function offsetUnset($offset) {
	unset($this->data[$offset]);
    }

    public function count() {
        return max(array_keys($this->data)) + 1;
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$js = <<<EOF
var jsarr = [ "one", , "three" ];
var_dump(0 in jsarr);
var_dump(1 in jsarr);

var_dump(0 in PHP.myarr);
var_dump(1 in PHP.myarr);

EOF;

$v8->executeString($js);

?>
===EOF===
--EXPECT--
bool(true)
bool(false)
bool(true)
bool(false)
===EOF===
