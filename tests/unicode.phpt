--TEST--
Test V8::executeString() : Check if imported code works with umlauts
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

# maybe more characters (e.g. from http://www.ltg.ed.ac.uk/~richard/unicode-sample.html?)
$unicode = 'äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃';

# insert unicode via snapshot
$snapshot = V8Js::createSnapshot("var snapshot = {unicode: '" . $unicode . "'}");

# start V8Js
$jscript = new V8Js('php', array(), array(), true, $snapshot);

# insert unicode via php var
$jscript->unicode = $unicode;

# insert unicode via executeString
$jscript->executeString("var execStr = {unicode: '" . $unicode . "'}");

# insert via module loader
$jscript->setModuleLoader(function ($path) use ($unicode) {
    return "module.exports = {unicode: '" . $unicode . "'}";
});


# return  to php
$jscript->executeString("values = {}");
$jscript->executeString("values['snapshot'] = snapshot.unicode");
$jscript->executeString("values['php'] = php.unicode");
$jscript->executeString("values['execStr'] = execStr.unicode");
$jscript->executeString("values['module'] = require('module').unicode");

$values = $jscript->executeString("values");

echo "snapshot: $values->snapshot\n";
echo "php     : $values->php\n";
echo "execStr : $values->execStr\n";
echo "module  : $values->module\n";

?>
===EOF===
--EXPECT--
snapshot: äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
php     : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
execStr : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
module  : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
===EOF===
