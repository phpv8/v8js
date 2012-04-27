--TEST--
Test V8::executeString() : Call JS from PHP
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
	
$a = new V8Js();

// Should not work with closure
$a->test = function ($params) { return (method_exists($params, 'cb1')) ? $params->cb1("hello") : false; };
$ret = $a->executeString('PHP.test(function (foo) { return foo + " world"; });');
var_dump(__LINE__, $ret);

// Test is_a()
$a->test = function ($params) { return (is_a($params, 'V8Object')) ? $params->cb1("hello") : false; };
$ret = $a->executeString('PHP.test({ "cb1" : function (foo) { return foo + " world"; } });');
var_dump(__LINE__, $ret);

// Test is_a()
$a->test = function ($params) { return (is_a($params, 'V8Function')) ? $params("hello") : false; };
$ret = $a->executeString('PHP.test(function (foo) { return foo + " world"; });');
var_dump(__LINE__, $ret);

// Should not work with object
$a->test = function ($params) { return (is_a($params, 'Closure')) ? $params("hello") : false; };
$ret = $a->executeString('PHP.test({ "cb1" : function (foo) { return foo + " world"; } });');
var_dump(__LINE__, $ret);

// Works
$a->test = function ($params) { return $params->cb1("hello"); };
$ret = $a->executeString('PHP.test({ "cb1" : function (foo) { return foo + " world"; } });');
var_dump(__LINE__, $ret);

?>
===EOF===
--EXPECT--
int(8)
bool(false)
int(13)
string(11) "hello world"
int(18)
string(11) "hello world"
int(23)
bool(false)
int(28)
string(11) "hello world"
===EOF===
