--TEST--
Test V8::executeString() : Export PHP methods on ArrayAccess objects
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
	echo 'count() = ', count($this->data), "\n";
        return count($this->data);
    }

    public function phpSidePush($value) {
	echo "push << $value\n";
	$this->data[] = $value;
    }

    public function push($value) {
	echo "php-side-push << $value\n";
	$this->data[] = $value;
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

/* Call PHP method to modify the array. */
$v8->executeString('PHP.myarr.phpSidePush(23);');

var_dump(count($v8->myarr));
var_dump($v8->myarr[3]);

/* And JS should see the changes due to live binding. */
$v8->executeString('var_dump(PHP.myarr.join(","));');


/* Call `push' method, this should trigger the PHP method. */
$v8->executeString('PHP.myarr.push(42);');

var_dump(count($v8->myarr));
var_dump($v8->myarr[4]);

/* And JS should see the changes due to live binding. */
$v8->executeString('var_dump(PHP.myarr.join(","));');

?>
===EOF===
--EXPECT--
push << 23
count() = 4
int(4)
int(23)
count() = 4
string(16) "one,two,three,23"
php-side-push << 42
count() = 5
int(5)
int(42)
count() = 5
string(19) "one,two,three,23,42"
===EOF===
