--TEST--
Test V8::executeString() : Prototype with PHP callbacks
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php 
$js = <<<'EOT'

String.prototype.test = function(){ return PHP.test(this.toString(), arguments); };
String.prototype.test_two = function(){ return PHP.test_two.__call('func', [this.toString(), arguments]); };
Array.prototype.test = function(){ return PHP.test(this.toString(), arguments); };
Array.prototype.test_two = function(){ return PHP.test_two.__call('func', [this.toString(), arguments]); };

"Foobar".test("foo", "bar");
"Foobar".test_two("foo", "bar");

["a","b","c"].test("foo", "bar");
["a","b","c"].test_two("foo", "bar");

EOT;

class A 
{
  public function __call($name, $args)
  {
    var_dump($args);
    return NULL;
  }
}

$a = new V8Js();
$a->test = function ($value) { var_dump(func_get_args()); return 'HELLO: ' . md5($value); };
$a->test_two = new A();
$a->executeString($js, 'foo');
?>
===EOF===
--EXPECTF--
array(2) {
  [0]=>
  string(6) "Foobar"
  [1]=>
  object(V8Object)#%d (2) {
    ["0"]=>
    string(3) "foo"
    ["1"]=>
    string(3) "bar"
  }
}
array(2) {
  [0]=>
  string(6) "Foobar"
  [1]=>
  object(V8Object)#%d (2) {
    ["0"]=>
    string(3) "foo"
    ["1"]=>
    string(3) "bar"
  }
}
array(2) {
  [0]=>
  string(5) "a,b,c"
  [1]=>
  object(V8Object)#%d (2) {
    ["0"]=>
    string(3) "foo"
    ["1"]=>
    string(3) "bar"
  }
}
array(2) {
  [0]=>
  string(5) "a,b,c"
  [1]=>
  object(V8Object)#%d (2) {
    ["0"]=>
    string(3) "foo"
    ["1"]=>
    string(3) "bar"
  }
}
===EOF===
