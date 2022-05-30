--TEST--
Test V8::executeString() : Issue #472 Destroy V8Js object which V8 isolate entered
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
class myjs extends \V8Js
{
   public function bosh()
   {
      $GLOBALS['v8test'] = null;
	  unset($GLOBALS['v8test']);
	}
}

$GLOBALS['v8test'] = new myjs('myjs');
$ret = $GLOBALS['v8test']->executeString('
	(() => {
		myjs.bosh()
	})
');

$ret();
var_dump($ret);
?>
===EOF===
--EXPECTF--
object(V8Function)#%d (0) {
}
===EOF===
