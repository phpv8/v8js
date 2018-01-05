--TEST--
Test V8Js::setModuleLoader : Native Module basic behaviour
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class NativeModule {
	public function sayHello($name)
	{
		echo "Hello $name!\n";
	}
}

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
	return new NativeModule();
});
$v8->executeString('require("foo").sayHello("World");');
?>
===EOF===
--EXPECT--
Hello World!
===EOF===