--TEST--
Test V8::executeString() : Check ArrayAccess interface wrapping
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    public function offsetExists($offset) {
        return $offset >= 0 && $offset <= 20;
    }

    public function offsetGet($offset) {
        return 19 - $offset;
    }

    public function offsetSet($offset, $value) {
        throw new Exception('Not implemented');
    }

    public function offsetUnset($offset) {
        throw new Exception('Not implemented');
    }

    public function count() {
        return 20;
    }
}

$myarr = new MyArray();
var_dump(count($myarr));
var_dump($myarr[5]);

$js = <<<EOJS
var_dump(PHP.myarr.constructor.name);
var_dump(PHP.myarr.length);
var_dump(PHP.myarr[5]);
EOJS;

$v8 = new V8Js();
$v8->myarr = (object) $myarr;
$v8->executeString($js);

?>
===EOF===
--EXPECT--
int(20)
int(14)
string(11) "ArrayAccess"
int(20)
int(14)
===EOF===
