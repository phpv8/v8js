--TEST--
Test V8::executeString() : Pthreads test #1
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');
if(!class_exists('Thread')) {
    die('skip pthreads extension required');
}
?>
--FILE--
<?php

class Workhorse extends Thread
{
    public function run()
    {
        $v8 = new V8Js();
        if($v8->executeString('(11 + 12)') != 23) {
            throw new \Exception('fail');
        }
    }
}

$foo = new Workhorse('foo');
$bar = new Workhorse('bar');

$foo->start();
$bar->start();

$foo->join();
$bar->join();
?>
===EOF===
--EXPECT--
===EOF===
