--TEST--
Test V8Js::setModuleNormaliser : Custom normalisation #005
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$v8->setModuleNormaliser(function ($base, $moduleName) {
    var_dump($base, $moduleName);
    if ($base == '' && $moduleName == './tags') {
        return ['./tags', 'index.js'];
    }
    if ($base == './tags' && $moduleName == './if.js') {
        return ['./tags', 'if.js'];
    }
    return [$base, $moduleName];
});

$v8->setModuleLoader(function ($moduleName) {
    print("setModuleLoader called for ".$moduleName."\n");
    switch ($moduleName) {
        case './app.js':
            return "require('./tags')";
        case './tags/index.js':
            return "require('./if.js')";
    }
});

$v8->executeString("require('./app.js')");

echo "------------------------------------------------\n";

$v8 = new V8Js();

$v8->setModuleNormaliser(function ($base, $moduleName) {
    var_dump($base, $moduleName);
    if ($base == '' && $moduleName == './tags') {
        return ['./tags', 'index.js'];
    }
    if ($base == './tags' && $moduleName == './if.js') {
        return ['./tags', 'if.js'];
    }
    return [$base, $moduleName];
});

$v8->setModuleLoader(function ($moduleName) {
    print("setModuleLoader called for ".$moduleName."\n");
    switch ($moduleName) {
        case './app.js':
            return "require('./tags')()"; // different
        case './tags/index.js':
            return "module.exports = function() {require('./if.js')}"; // different
    }
});

$v8->executeString("require('./app.js')");

?>
===EOF===
--EXPECT--
string(0) ""
string(8) "./app.js"
setModuleLoader called for ./app.js
string(0) ""
string(6) "./tags"
setModuleLoader called for ./tags/index.js
string(6) "./tags"
string(7) "./if.js"
setModuleLoader called for ./tags/if.js
------------------------------------------------
string(0) ""
string(8) "./app.js"
setModuleLoader called for ./app.js
string(0) ""
string(6) "./tags"
setModuleLoader called for ./tags/index.js
string(6) "./tags"
string(7) "./if.js"
setModuleLoader called for ./tags/if.js
===EOF===
