--TEST--
Test V8Js::createSnapshot() : Basic snapshot creation & re-use
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

if (!method_exists('V8Js', 'createSnapshot')) {
    die('SKIP V8Js::createSnapshot not supported');
}
?>
--FILE--
<?php
$doublifySource = <<<EOJS
function doublify(x) {
    return 2 * x;
}
EOJS;

$snap = V8Js::createSnapshot($doublifySource);

if (strlen($snap) > 0) {
    var_dump("snapshot successfully created");
}

$v8 = new V8Js('PHP', array(), array(), true, $snap);
$v8->executeString('var_dump(doublify(23));');
?>
===EOF===
--EXPECT--
string(29) "snapshot successfully created"
int(46)
===EOF===
