--TEST--
Test V8::executeString() : Delete (unset) ArrayAccess keys
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php
class MyArray implements ArrayAccess, Countable {
    private $data = Array('one', 'two', 'three');

    public function offsetExists($offset): bool {
        return isset($this->data[$offset]);
    }

    public function offsetGet(mixed $offset): mixed {
        if(!$this->offsetExists($offset)) {
         return null;
        }
        return $this->data[$offset];
    }

    public function offsetSet(mixed $offset, mixed $value): void {
        $this->data[$offset] = $value;
    }

    public function offsetUnset(mixed $offset): void {
        unset($this->data[$offset]);
    }

    public function count(): int {
        return max(array_keys($this->data)) + 1;
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$js = <<<EOF
var jsarr = [ "one", "two", "three" ];
delete jsarr[1];
var_dump(jsarr.join(","));

delete PHP.myarr[1];
var_dump(PHP.myarr.join(","));

EOF;

$v8->executeString($js);

?>
===EOF===
--EXPECT--
string(10) "one,,three"
string(10) "one,,three"
===EOF===
