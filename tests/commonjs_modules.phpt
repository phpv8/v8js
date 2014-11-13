--TEST--
Test V8Js::setModuleLoader : CommonJS modules
--SKIPIF--
<?php
if(!function_exists('json_encode')) {
  die('SKIP');
}
require_once(dirname(__FILE__) . '/skipif.inc');
?>
--FILE--
<?php

$JS = <<< EOT
require("path/to/module1");
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
  switch ($module) {
    case 'path/to/module1':
      return 'print(' . json_encode($module . PHP_EOL) . ');require("./module2");';

    case 'path/to/module2':
      return 'print(' . json_encode($module . PHP_EOL) . ');require("../../module3");';

    default:
      return 'print(' . json_encode($module . PHP_EOL) . ');';
  }
});

$v8->executeString($JS, 'module.js');
?>
===EOF===
--EXPECT--
path/to/module1
path/to/module2
module3
===EOF===
