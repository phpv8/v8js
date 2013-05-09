--TEST--
Test V8::executeString() : Time limit
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$JS = <<< EOT
var text = "abcdefghijklmnopqrstuvwyxz0123456789";
var memory = "";
for (var i = 0; i < 1000000; ++i) {
    memory += text;
}
EOT;

$v8 = new V8Js();

try {
    var_dump($v8->executeString($JS, 'basic.js', V8Js::FLAG_NONE, 0, 10000000));
} catch (V8JsMemoryLimitException $e) {
    var_dump($e);
}
?>
===EOF===
--EXPECT--
object(V8JsMemoryLimitException)#2 (7) {
  ["message":protected]=>
  string(46) "Script memory limit of 10000000 bytes exceeded"
  ["string":"Exception":private]=>
  string(0) ""
  ["code":protected]=>
  int(0)
  ["file":protected]=>
  string(36) "/var/www/v8js/tests/memory_limit.php"
  ["line":protected]=>
  int(13)
  ["trace":"Exception":private]=>
  array(1) {
    [0]=>
    array(6) {
      ["file"]=>
      string(36) "/var/www/v8js/tests/memory_limit.php"
      ["line"]=>
      int(13)
      ["function"]=>
      string(13) "executeString"
      ["class"]=>
      string(4) "V8Js"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(5) {
        [0]=>
        string(125) "var text = "abcdefghijklmnopqrstuvwyxz0123456789";
var memory = "";
for (var i = 0; i < 1000000; ++i) {
    memory += text;
}"
        [1]=>
        string(8) "basic.js"
        [2]=>
        int(1)
        [3]=>
        int(0)
        [4]=>
        int(10000000)
      }
    }
  }
  ["previous":"Exception":private]=>
  NULL
}
===EOF===