--TEST--
Test V8::executeString() : Generators PHP -> V8
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

function TheGenerator()
{
    for($i = 0; $i < 4; $i ++) {
	yield $i;
    }
}

foreach(TheGenerator() as $i) {
    var_dump($i);
}


$js = <<<EOJS
for(var i of PHP.gen) {
  var_dump(i);
}
EOJS;

$v8 = new V8Js();
$v8->gen = TheGenerator();
$gen = $v8->executeString($js);

?>
===EOF===
--EXPECTF--
int(0)
int(1)
int(2)
int(3)
int(0)
int(1)
int(2)
int(3)
===EOF===
