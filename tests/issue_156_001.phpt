--TEST--
Test V8::executeString() : Backwards compatibility for issue #156
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--INI--
v8js.compat_php_exceptions = 1
--FILE--
<?php

$v8 = new V8Js();

$v8->throwPHPException = function () {
    echo "throwing PHP exception now ...\n";
    throw new \Exception('foo');
};

$JS = <<< EOT
PHP.throwPHPException();
print("... old behaviour was to not stop JS execution on PHP exceptions\\n");
EOT;

try {
    $v8->executeString($JS, 'issue_156_001.js');
} catch(Exception $e) {
    var_dump($e->getMessage());
}
?>
===EOF===
--EXPECT--
throwing PHP exception now ...
... old behaviour was to not stop JS execution on PHP exceptions
string(3) "foo"
===EOF===
