<?php

V8Js::registerExtension('a', file_get_contents('js/json-template.js'), array('b'));
V8Js::registerExtension('b', file_get_contents('js/jstparser.js'), array('a'));

var_dump(V8JS::getExtensions());

$a = new V8Js('myobj', array(), array('b'));

