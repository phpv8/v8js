--TEST--
Test V8::executeString() : Time limit
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
var text = "abcdefghijklmnopqrstuvwyxz0123456789";
for (var i = 0; i < 10000000; ++i) {
    var encoded = encodeURI(text);
}
EOT;

$v8 = new V8Js();

try {
    var_dump($v8->executeString($JS, 'basic.js', V8Js::FLAG_NONE, 1000));
} catch (V8JsTimeLimitException $e) {
    var_dump($e);
}
?>
===EOF===
--EXPECT--
object(V8JsTimeLimitException)#2 (7) {
  ["message":protected]=>
  string(47) "Script time limit of 1000 milliseconds exceeded"
  ["string":"Exception":private]=>
  string(0) ""
  ["code":protected]=>
  int(0)
  ["file":protected]=>
  string(34) "/var/www/v8js/tests/time_limit.php"
  ["line":protected]=>
  int(13)
  ["trace":"Exception":private]=>
  array(1) {
    [0]=>
    array(6) {
      ["file"]=>
      string(34) "/var/www/v8js/tests/time_limit.php"
      ["line"]=>
      int(13)
      ["function"]=>
      string(13) "executeString"
      ["class"]=>
      string(4) "V8Js"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(4) {
        [0]=>
        string(124) "var text = "abcdefghijklmnopqrstuvwyxz0123456789";
for (var i = 0; i < 10000000; ++i) {
    var encoded = encodeURI(text);
}"
        [1]=>
        string(8) "basic.js"
        [2]=>
        int(1)
        [3]=>
        int(1000)
      }
    }
  }
  ["previous":"Exception":private]=>
  NULL
}
===EOF===
