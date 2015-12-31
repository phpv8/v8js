--TEST--
Test V8Js::setModuleLoader : Module source naming
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$JS = <<< EOT
require('./foo//bar');
EOT;

$v8 = new V8Js();
$v8->setModuleLoader(function($module) {
    // return code with syntax errors to provoke script exception
    return "foo(blar);";
});

try {
    $v8->executeString($JS, 'commonjs_source_naming.js');
} catch (V8JsScriptException $e) {
    var_dump($e->getJsFileName());
}
?>
===EOF===
--EXPECT--
string(7) "foo/bar"
===EOF===
