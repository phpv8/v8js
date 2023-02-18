--TEST--
Test empty() : Segmentation fault caused by 'empty' check on a V8Function object
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
$v = new \V8Js();
$r = $v->executeString('
    a = {
	    main: function() {}
    };
', null, V8Js::FLAG_FORCE_ARRAY | V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);

if (!empty($r['main'])) {
    echo 'Ok' . PHP_EOL;
}
?>
--EXPECTF--
Ok

