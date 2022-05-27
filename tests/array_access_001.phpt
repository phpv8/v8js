--TEST--
Test V8::executeString() : Check ArrayAccess live binding
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

if (PHP_VERSION_ID < 80000) {
    class MyArray implements ArrayAccess, Countable {
        private $data = Array('one', 'two', 'three');

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

        public function push($value) {
        $this->data[] = $value;
        }
    }
} else {
    class MyArray implements ArrayAccess, Countable {
        private $data = Array('one', 'two', 'three');

        public function offsetExists($offset): bool {
            return isset($this->data[$offset]);
        }

        public function offsetGet(mixed $offset): mixed {
            return $this->data[$offset];
        }

        public function offsetSet(mixed $offset, mixed $value): void {
            $this->data[$offset] = $value;
        }

        public function offsetUnset(mixed $offset): void {
            throw new Exception('Not implemented');
        }

        public function count(): int {
            return count($this->data);
        }

        public function push($value) {
        $this->data[] = $value;
        }
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$v8->executeString('var_dump(PHP.myarr.join(","));');

/* array is "live bound", i.e. new elements just pop up on js side. */
$v8->myarr->push('new');
$v8->executeString('var_dump(PHP.myarr.join(","));');

?>
===EOF===
--EXPECT--
string(13) "one,two,three"
string(17) "one,two,three,new"
===EOF===
