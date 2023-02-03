--TEST--
Test V8::executeString() : Issue #497 (segmentation fault calling PHP exit inside object function)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
class Foo {
    function __destruct() {
	    var_dump('Foo __destruct');
    }

    function somecall() {
	    var_dump('Foo somecall');
    }

    function bar() {
	    global $v8;
	    var_dump('Foo bar');
    	exit;
    }
}

$v8 = new \V8Js();
$v8->foo = new Foo();

$JS = <<< EOT
PHP.foo.somecall();
PHP.foo.bar();
EOT;

$v8->executeString($JS, '', \V8JS::FLAG_PROPAGATE_PHP_EXCEPTIONS);
echo 'Not here!!';
?>
--EXPECTF--
string(12) "Foo somecall"
string(7) "Foo bar"
string(14) "Foo __destruct"
