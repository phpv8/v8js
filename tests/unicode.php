--TEST
Test V8::executeString() : Check if imported code works with some unicode symbols
--SKIPIF--
<?php
# check if v8js was compiled with snapshot support
define('V8_WITH_SNAPSHOT', method_exists("V8Js", "createSnapshot"));

# maybe more characters (e.g. from http://www.ltg.ed.ac.uk/~richard/unicode-sample.html?)
$unicode = 'äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃';

# insert unicode via snapshot
if (V8_WITH_SNAPSHOT) {
    $snapshot = V8Js::createSnapshot("var snapshot = {unicode: '" . $unicode . "'}");
} else {
    # argument is only checked for type and further ignored
    $snapshot = '';
}

# start V8Js
$jscript = new V8Js('php', array(), array(), true, $snapshot);

# insert unicode via php var
$jscript->unicode = $unicode;

# insert unicode via executeString
$jscript->executeString("var execStr = {unicode: '" . $unicode . "'}");

# return  to php
$jscript->executeString("values = {}");
if (V8_WITH_SNAPSHOT) {
    $jscript->executeString("values['snapshot'] = snapshot.unicode");
} else {
  # shim this test
  $jscript->executeString("values['snapshot'] = '" . $unicode . "'");
}
$jscript->executeString("values['php'] = php.unicode");
$jscript->executeString("values['execStr'] = execStr.unicode");
$values = $jscript->executeString("values");

echo "snapshot: $values->snapshot\n";
echo "php     : $values->php\n";
echo "execStr : $values->execStr\n";
?>
===EOF===
--EXPECT--
snapshot: äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
php     : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
execStr : äöüßÜÄÖÜß€áàâÁÀÂµ²³▁▂▃▄▅▆▇█    ㌀ ㌁ ㌂ ㌃
===EOF===
