--TEST--
Test V8::executeString() : Regression #121 Z_ADDREF_P
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();
$v8->blar = new stdClass();

$js = <<<EOT
PHP.blar.blub = {}
PHP.blar.blub['foo'] = 23;
PHP.blar.blub['bar'] = 5;
PHP.blar.blub['baz'] = 42;

print("Hello World!\\n");
EOT;

$v8->executeString($js);
?>
===EOF===
--EXPECT--
Hello World!
===EOF===
