--TEST
Test V8::executeString() : Check if imported code works with some unicode symbols
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
# check if snapshots are supported
define('V8_WITH_SNAPSHOT', method_exists("V8Js", "createSnapshot"));

# maybe more characters (e.g. from http://www.ltg.ed.ac.uk/~richard/unicode-sample.html?)
$unicode = 'äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃';
$moduleFileBase = 'test';

# insert unicode via snapshot
if (V8_WITH_SNAPSHOT) {
    $snapshot = V8Js::createSnapshot("var snapshot = {unicode: '" . $unicode . "'}");
} else {
    $snapshot = '';
}

# start V8Js
$jscript = new V8Js('php', array(), array(), true, $snapshot);
$jscript->setModuleLoader(function ($path) {
    return file_get_contents($path . ".js");
});

# insert unicode via php var
$jscript->unicode = $unicode;

# insert unicode via executeString
$jscript->executeString("var execStr = {unicode: '" . $unicode . "'}");

# insert unicode via commonJS module
file_put_contents("./$moduleFileBase.js", "module.exports = {unicode: '$unicode'}");
$jscript->executeString("var module = require('./$moduleFileBase')");

# return  to php
$jscript->executeString("values = {}");
if (V8_WITH_SNAPSHOT) {
    $jscript->executeString("values['snapshot'] = snapshot.unicode");
} else {
  // if snapshots are not compiled shim this test
  $jscript->executeString("values['snapshot'] = '" . $unicode . "'");
}
$jscript->executeString("values['php'] = php.unicode");
$jscript->executeString("values['execStr'] = execStr.unicode");
$jscript->executeString("values['module'] = module.unicode");
$values = $jscript->executeString("values");

echo "snapshot: $values->snapshot\n";
echo "php     : $values->php\n";
echo "execStr : $values->execStr\n";
echo "module  : $values->module\n";
?>
--EXPECT--
snapshot: äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
php     : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
execStr : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
--CLEAN--
<?php
unlink("./$moduleFileBase.js");
?>
