--TEST--
Test V8Js::setModuleLoader : this === module.exports
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->setModuleLoader(function ($moduleName) {
    return <<<'EOJS'
        var_dump(this === global);
        var_dump(this === module.exports);
EOJS
    ;
});

$v8->executeString(<<<'EOJS'
    var result = require('foo');
EOJS
);

?>
===EOF===
--EXPECT--
bool(false)
bool(true)
===EOF===
