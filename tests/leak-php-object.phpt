--TEST--
Test V8::executeString() : Test for leaked PHP object if passed back multiple times
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$js =<<< EOF
// Generate a large number of objects so we trigger definitely trigger the
// garbage collector.  5000 * 1 kB should be enough.
for(var i = 0; i < 5000; i ++) {
    PHP.foo.getStdClassObject();
}
EOF;

class Foo {
    public function getStdClassObject() {
        return new stdClass();
    }

    public function __destruct() {
        echo "destroyed\n";
    }
}

$v8 = new V8Js();
$v8->foo = new Foo();
$v8->executeString($js);
unset($v8);

?>
===EOF===
--EXPECT--
destroyed
===EOF===
