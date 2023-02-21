--TEST--
Test V8::executeString() : redirects toJSON() to jsonSerialize
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
class Foo implements JsonSerializable {
    public function jsonSerialize(): mixed {
    	return ['foo', 'bar'];
    }
}

$v8 = new V8Js;
$v8->foo = new Foo;
$v8->executeString('var_dump(JSON.stringify(PHP.foo));');
?>
===EOF===
--EXPECTF--
string(13) "["foo","bar"]"
===EOF===