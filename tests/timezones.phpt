--TEST--
Test V8::executeString() : Check timezone handling
--SKIPIF--
<?php
if(strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
	die('SKIP TZ not handled by v8 on Windows');
}
require_once(dirname(__FILE__) . '/skipif.inc');

ob_start(NULL, 0, PHP_OUTPUT_HANDLER_CLEANABLE | PHP_OUTPUT_HANDLER_REMOVABLE);
phpinfo(INFO_MODULES);
$minfo = ob_get_contents();
ob_end_clean();
if(preg_match("/V8 Engine Linked Version => (.*)/", $minfo, $matches)) {
	$version = explode('.', $matches[1]);
	if($version[0] < 7 || ($version[0] == 7 && $version[1] < 5)) {
		die('SKIP Only for V8 version >= 7.5');
	}
}
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
Thu Mar 20 2014 11:03:24 GMT+0200 (Eastern European Standard Time)
Thu Mar 20 2014 05:03:24 GMT-0400 (Eastern Daylight Time)
Thu Mar 20 2014 11:03:24 GMT+0200 (Eastern European Standard Time)
===EOF===
