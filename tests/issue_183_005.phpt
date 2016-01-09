--TEST--
Test V8::executeString() : Method access on derived classes (__sleep)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo extends \V8Js
{
    public function __sleep()
    {
	var_dump("foo");
    }
}

?>
===EOF===
--EXPECTF--
Fatal error: Cannot override final method V8Js::__sleep() in %s
