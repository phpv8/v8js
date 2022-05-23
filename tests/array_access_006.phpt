--TEST--
Test V8::executeString() : Enumerate ArrayAccess keys
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    private $data = Array('one', 'two', 'three', null, 'five');

    public function offsetExists($offset): bool {
	return isset($this->data[$offset]);
    }

    public function offsetGet($offset): mixed {
	return $this->data[$offset];
    }

    public function offsetSet($offset, $value): void {
	echo "set[$offset] = $value\n";
	$this->data[$offset] = $value;
    }

    public function offsetUnset($offset): void {
        throw new Exception('Not implemented');
    }

    public function count(): int {
        return count($this->data);
    }
}

$v8 = new V8Js();
$v8->myarr = new MyArray();

$js = <<<EOF
var jsarr = [ "one", "two", "three", , "five" ];
for(var i in jsarr) {
  var_dump(i);
}

for(var i in PHP.myarr) {
  var_dump(i);
}

EOF;

$v8->executeString($js);

?>
===EOF===
--EXPECT--
string(1) "0"
string(1) "1"
string(1) "2"
string(1) "4"
string(1) "0"
string(1) "1"
string(1) "2"
string(1) "4"
===EOF===
