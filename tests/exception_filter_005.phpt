--TEST--
Test V8::setExceptionFilter() : Uninstall filter on NULL
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
	return "moep";
});

$v8->executeString('
	try {
		PHP.throwException("Oops");
	}
	catch (e) {
		var_dump(e);
	}
', null, V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);

$v8->setExceptionFilter(null);

try {
        $v8->executeString('
                try {
                        PHP.throwException("Oops");
                        print("done\\n");
                }
                catch (e) {
                        print("caught\\n");
                        var_dump(e.getMessage());
                }
        ', null, V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
} catch (Exception $ex) {
        echo "caught in php: " . $ex->getMessage() . PHP_EOL;
}

?>
===EOF===
--EXPECT--
exception filter called.
string(4) "moep"
caught
string(4) "Oops"
===EOF===
