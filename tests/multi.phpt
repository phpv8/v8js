--TEST--
Test V8::executeString() : Use multiple V8js instances
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$instances = array();
for($i = 0; $i < 5; $i ++) {
	$v8 = new V8Js();
	$v8->executeString('var instNo = '.$i);
	$instances[] = $v8;
}

$JS = <<< EOT
len = print('Hello' + ' ' + 'World!  This is instance ' + instNo + "\\n");
len;
EOT;

foreach($instances as $v8) {
	$v8->executeString($JS, 'basic.js');
}

?>
===EOF===
--EXPECT--
Hello World!  This is instance 0
Hello World!  This is instance 1
Hello World!  This is instance 2
Hello World!  This is instance 3
Hello World!  This is instance 4
===EOF===
