--TEST--
Test V8Js::setModuleLoader : exports/module.exports behaviour
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->setModuleLoader(function ($moduleName) {
    return <<<'EOJS'
        var_dump(typeof exports);
        var_dump(typeof module.exports);

        // for compatibility both should be linked
        var_dump(exports === module.exports);

        exports = { number: 23 };
        module.exports = { number: 42 };
EOJS
    ;
});

$v8->executeString(<<<'EOJS'
    var result = require('foo');

    // expect module.exports value to be picked up
    var_dump(typeof result);
    var_dump(result.number);
EOJS
);

?>
===EOF===
--EXPECT--
string(6) "object"
string(6) "object"
bool(true)
string(6) "object"
int(42)
===EOF===
