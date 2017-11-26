--TEST--
Test V8Js::setModuleLoader : modules can return arbitrary values
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->setModuleLoader(function ($moduleName) {
    return <<<'EOJS'
        module.exports = 23;
EOJS
    ;
});

$v8->executeString(<<<'EOJS'
    var result = require('foo');
    var_dump(typeof result);
    var_dump(result);
EOJS
);

?>
===EOF===
--EXPECT--
string(6) "number"
int(23)
===EOF===
