--TEST--
Test V8::executeString() : Generators PHP -> V8 (instantite in JS)
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');

// Actually this check is a bit bad as it tests import, but currently
// there is no flag we can check for export
if (!class_exists('V8Generator')) {
    die("skip Installed V8 version doesn't support generators");
}
?>
--FILE--
<?php

$js = <<<EOJS
for(var i of PHP.Gen()) {
    var_dump(i);
}
EOJS;

$v8 = new V8Js();
$v8->Gen = function() {
    for($i = 0; $i < 4; $i ++) {
        yield $i;
    }
};

$v8->executeString($js);

?>
===EOF===
--EXPECTF--
int(0)
int(1)
int(2)
int(3)
===EOF===
