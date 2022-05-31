--TEST--
Test V8::setExceptionProxyFactory() : String conversion
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
class myv8 extends V8Js
{
	public function throwException(string $message) {
		throw new Exception($message);
	}
}

$v8 = new myv8();
$v8->setExceptionProxyFactory(function (Throwable $ex) {
	echo "exception proxy factory called.\n";
	return $ex->getMessage();
});

$v8->executeString('
	try {
		PHP.throwException("Oops");
	}
	catch (e) {
		var_dump(typeof e); // string
		var_dump(e);
	}
', null, V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
?>
===EOF===
--EXPECT--
exception proxy factory called.
string(6) "string"
string(4) "Oops"
===EOF===
