--TEST--
Test V8::executeString() : Check ArrayAccess interface wrapping
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.use_array_access = 1
--FILE--
<?php

class MyArray implements ArrayAccess, Countable {
    public function offsetExists($offset): bool {
        return $offset >= 0 && $offset <= 20;
    }

    public function offsetGet(mixed $offset): mixed {
        return 19 - $offset;
    }

    public function offsetSet(mixed $offset, mixed $value): void {
        throw new Exception('Not implemented');
    }

    public function offsetUnset(mixed $offset): void {
        throw new Exception('Not implemented');
    }

    public function count(): int {
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
var_dump(PHP.myarr.join(', '));

var_dump(PHP.myarr.slice(5, 10).join(', '));
EOJS;

$v8 = new V8Js();
$v8->myarr = $myarr;
$v8->executeString($js);

?>
===EOF===
--EXPECT--
int(20)
int(14)
string(5) "Array"
int(20)
int(14)
string(68) "19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0"
string(18) "14, 13, 12, 11, 10"
===EOF===
