--TEST--
Test V8::executeString() : Check array access setter behaviour
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    private $data = array('one', 'two', 'three');

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
        throw new Exception('Not implemented');
    }

    public function count() {
        return count($this->data);
    }
}

$myarr = new MyArray();
$myarr[0] = 'three';

$js = <<<EOJS
var_dump(PHP.myarr[2]);
PHP.myarr[2] = 'one';
var_dump(PHP.myarr[2]);
var_dump(PHP.myarr.join(','));
EOJS;

$v8 = new V8Js();
$v8->myarr = $myarr;
$v8->executeString($js);

var_dump($myarr[2]);

?>
===EOF===
--EXPECT--
string(5) "three"
string(3) "one"
string(13) "three,two,one"
string(3) "one"
===EOF===
