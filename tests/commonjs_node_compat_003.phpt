--TEST--
Test V8Js::setModuleLoader : delete module.exports yields undefined
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->setModuleLoader(function ($moduleName) {
    return <<<'EOJS'
        delete module.exports;
EOJS
    ;
});

$v8->executeString(<<<'EOJS'
    var result = require('foo');
    var_dump(typeof result);
EOJS
);

?>
===EOF===
--EXPECT--
string(9) "undefined"
===EOF===
