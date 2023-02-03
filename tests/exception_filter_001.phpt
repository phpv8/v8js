--TEST--
Test V8::setExceptionFilter() : String conversion
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
$v8->setExceptionFilter(function (Throwable $ex) {
	echo "exception filter called.\n";
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
exception filter called.
string(6) "string"
string(4) "Oops"
===EOF===
