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

$a->test = function ($params) { var_dump($params); return $params->cb1("hello"); };
$ret = $a->executeString('PHP.test({ "cb1" : function (foo) { return foo + " world"; } });');
var_dump(__LINE__, $ret);

// FIX! method_exists() Leaks!
$a->test = function ($params) { var_dump($params, method_exists($params, 'cb1'), $params->cb1); };
$ret = $a->executeString('PHP.test({ "cb1" : function (foo) { return foo + " world"; } });');

