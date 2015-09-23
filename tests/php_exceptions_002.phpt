--TEST--
Test V8::executeString() : PHP Exception handling (multi-level)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class Foo {
    function throwException() {
	throw new \Exception("Test-Exception");
    }

    function recurse($i) {
        echo "recurse[$i] ...\n";
        global $work;
        $work($i);
    }
}

$v8 = new V8Js();
$v8->foo = new \Foo();

$work = $v8->executeString(<<<EOT
var work = function(level) {
  if(level--) {
    PHP.foo.recurse(level);
  }
  else {
    PHP.foo.throwException();
  }
};
work;
EOT
);

for($i = 0; $i < 5; $i ++) {
    var_dump($i);
    try {
        $work($i);
    } catch (Exception $e) {
        var_dump($e->getMessage());
    }
}
?>
===EOF===
--EXPECT--
int(0)
string(14) "Test-Exception"
int(1)
recurse[0] ...
string(14) "Test-Exception"
int(2)
recurse[1] ...
recurse[0] ...
string(14) "Test-Exception"
int(3)
recurse[2] ...
recurse[1] ...
recurse[0] ...
string(14) "Test-Exception"
int(4)
recurse[3] ...
recurse[2] ...
recurse[1] ...
recurse[0] ...
string(14) "Test-Exception"
===EOF===
