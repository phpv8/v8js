--TEST--
Test V8::setExceptionFilter() : re-throw exception in exception filter
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
	// re-throw exception so it is not forwarded
        throw $ex;
});

try {
        $v8->executeString('
                try {
                        PHP.throwException("Oops");
                        print("done\\n");
                }
                catch (e) {
                        print("caught\\n");
                        var_dump(e);
                }
        ', null, V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
} catch (Exception $ex) {
        echo "caught in php: " . $ex->getMessage() . PHP_EOL;
}
?>
===EOF===
--EXPECT--
caught in php: Oops
===EOF===
