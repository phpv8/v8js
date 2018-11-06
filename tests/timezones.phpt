--TEST--
Test V8::executeString() : Check timezone handling
--SKIPIF--
SKIP test currently broken, see #378

<?php
if(strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
	die('SKIP TZ not handled by v8 on Windows');
}
require_once(dirname(__FILE__) . '/skipif.inc');
?>
--FILE--
<?php

$v8 = new V8Js();
try {
	putenv('TZ=Europe/Helsinki');
	$v8->executeString('print (new Date("Thu, 20 Mar 2014 09:03:24 +0000")).toString();');
	echo "\n";
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
	putenv('TZ=America/New_York');
	$v8->executeString('print (new Date("Thu, 20 Mar 2014 09:03:24 +0000")).toString();');
	echo "\n";
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}

try {
	putenv('TZ=Europe/Helsinki');
	$v8->executeString('print (new Date("Thu, 20 Mar 2014 09:03:24 +0000")).toString();');
	echo "\n";
} catch (V8JsScriptException $e) {
	var_dump($e->getMessage());
}
?>
===EOF===
--EXPECT--
Thu Mar 20 2014 11:03:24 GMT+0200 (EET)
Thu Mar 20 2014 05:03:24 GMT-0400 (EDT)
Thu Mar 20 2014 11:03:24 GMT+0200 (EET)
===EOF===
