--TEST--
Test V8::executeString() : Forcing to arrays (argument passing)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php 

$js = <<<'EOT'
String.prototype.test = function(){ return PHP.test(this, arguments); };
"Foobar".test("foo", "bar");
EOT;

$v8 = new V8Js();
$v8->test = function ($value) { var_dump(func_get_args()); };

try {
	$v8->executeString($js, 'no_flags.js');
	echo "---\n";
	$v8->executeString($js, 'force_to_array.js', V8Js::FLAG_FORCE_ARRAY);
} catch (V8JsScriptException $e) {
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
array(2) {
  [0]=>
  object(V8Object)#%d (6) {
    ["0"]=>
    string(1) "F"
    ["1"]=>
    string(1) "o"
    ["2"]=>
    string(1) "o"
    ["3"]=>
    string(1) "b"
    ["4"]=>
    string(1) "a"
    ["5"]=>
    string(1) "r"
  }
  [1]=>
  object(V8Object)#%d (2) {
    ["0"]=>
    string(3) "foo"
    ["1"]=>
    string(3) "bar"
  }
}
---
array(2) {
  [0]=>
  array(6) {
    [0]=>
    string(1) "F"
    [1]=>
    string(1) "o"
    [2]=>
    string(1) "o"
    [3]=>
    string(1) "b"
    [4]=>
    string(1) "a"
    [5]=>
    string(1) "r"
  }
  [1]=>
  array(2) {
    [0]=>
    string(3) "foo"
    [1]=>
    string(3) "bar"
  }
}
===EOF===
