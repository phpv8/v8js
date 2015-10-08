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
    protected $val;

    public function __construct($val)
    {
        $this->val = $val;
    }

    public function run()
    {
        $v8 = new V8Js();
        $v8->val = $this->val;
        $v8->sync_var_dump = function($value) {
            $this->synchronized(function($thread) use ($value) {
                    while(!$thread->readyToPrint) {
                        $thread->wait();
                    }
                    var_dump($value);
                    $thread->notify();
                }, $this);
        };

        $v8->executeString('PHP.sync_var_dump(PHP.val);');
    }
}

$foo = new Workhorse('foo');
$bar = new Workhorse('bar');

$foo->start();
$bar->start();

$bar->synchronized(function($thread) {
    $thread->readyToPrint = true;
    $thread->notify();
    $thread->wait();
}, $bar);

$foo->synchronized(function($thread) {
    $thread->readyToPrint = true;
    $thread->notify();
    $thread->wait();
}, $foo);

$foo->join();
$bar->join();
?>
===EOF===
--EXPECT--
string(3) "bar"
string(3) "foo"
===EOF===