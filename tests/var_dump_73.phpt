--TEST--
Test V8::executeString() : var_dump
--SKIPIF--
<?php
require_once(dirname(__FILE__) . '/skipif.inc');
if (PHP_VERSION_ID < 70300) die('skip Only for php version >= 7.3');
?>
--INI--
date.timezone=UTC
--FILE--
<?php
# Test var_dump of various types

$JS = <<< EOT

print("--- JS var_dump of PHP object ----\\n");
var_dump(PHP.phptypes);

print("--- JS var_dump of JS object ----\\n");
var types = {
	undefined: undefined,
	null: null,
	bool: true,
	string: "string",
	uint: 1,
	int: -1,
	number: 3.141592654,
	// XXX this gets parsed with local timezone,
	//     which is bad for test repeatability.
	//date: new Date('September 27, 1976 09:00:00 GMT'),
	regexp: /regexp/,
	array: [1,2,3],
	object: { field: "foo" },
	function: function id(x) { return x; },
	phpobject: PHP.obj
};

var_dump(types);
print("--- PHP var_dump of JS object ----\\n");
types;
EOT;

class Foo {
	  var $field = "php";
}

$v8 = new V8Js();
$v8->obj = new Foo;

$phptypes = $v8->phptypes = array(
	"null" => NULL,
	"bool" => true,
	"string" => "string",
	"uint" => 1,
	"int" => -1,
	"number" => 3.141592654,
	"date" => new DateTime('September 27, 1976 09:00:00 UTC', new DateTimeZone('UTC')),
	//"regexp" => new Regexp('/regexp/'), /* no native PHP regex type */
	"array" => array(1,2,3),
	"object" => array( "field" => "foo" ),
	"function" => (function ($x) { return $x; }),
	"phpobject" => new Foo
);

echo "---- PHP var_dump of PHP object ----\n";
var_dump($phptypes);

try {
	var_dump($v8->executeString($JS, 'var_dump.js'));
} catch (V8JsScriptException $e) {
	echo "Error!\n";
	var_dump($e);
}
?>
===EOF===
--EXPECTF--
---- PHP var_dump of PHP object ----
array(11) {
  ["null"]=>
  NULL
  ["bool"]=>
  bool(true)
  ["string"]=>
  string(6) "string"
  ["uint"]=>
  int(1)
  ["int"]=>
  int(-1)
  ["number"]=>
  float(3.141592654)
  ["date"]=>
  object(DateTime)#%d (3) {
    ["date"]=>
    string(%d) "1976-09-27 09:00:00%r(\.0+)?%r"
    ["timezone_type"]=>
    int(3)
    ["timezone"]=>
    string(3) "UTC"
  }
  ["array"]=>
  array(3) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [2]=>
    int(3)
  }
  ["object"]=>
  array(1) {
    ["field"]=>
    string(3) "foo"
  }
  ["function"]=>
  object(Closure)#%d (1) {
    ["parameter"]=>
    array(1) {
      ["$x"]=>
      string(10) "<required>"
    }
  }
  ["phpobject"]=>
  object(Foo)#%d (1) {
    ["field"]=>
    string(3) "php"
  }
}
--- JS var_dump of PHP object ----
array (11) {
  ["null"] =>
  NULL
  ["bool"] =>
  bool(true)
  ["string"] =>
  string(6) "string"
  ["uint"] =>
  int(1)
  ["int"] =>
  int(-1)
  ["number"] =>
  float(3.141593)
  ["date"] =>
  object(DateTime)#%d (19) {
    ["createFromImmutable"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["createFromFormat"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["getLastErrors"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["format"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["modify"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["add"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["sub"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["getTimezone"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["setTimezone"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["getOffset"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["setTime"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["setDate"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["setISODate"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["setTimestamp"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["getTimestamp"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["diff"] =>
    object(Closure)#%d {
        function () { [native code] }
    }
    ["$date"] =>
    string(%d) "1976-09-27 09:00:00%r(\.0+)?%r"
    ["$timezone_type"] =>
    int(3)
    ["$timezone"] =>
    string(3) "UTC"
  }
  ["array"] =>
  array(3) {
    [0] =>
    int(1)
    [1] =>
    int(2)
    [2] =>
    int(3)
  }
  ["object"] =>
  array (1) {
    ["field"] =>
    string(3) "foo"
  }
  ["function"] =>
  object(Closure)#%d (0) {
  }
  ["phpobject"] =>
  object(Foo)#%d (1) {
    ["$field"] =>
    string(3) "php"
  }
}
--- JS var_dump of JS object ----
object(Object)#%d (12) {
  ["undefined"] =>
  NULL
  ["null"] =>
  NULL
  ["bool"] =>
  bool(true)
  ["string"] =>
  string(6) "string"
  ["uint"] =>
  int(1)
  ["int"] =>
  int(-1)
  ["number"] =>
  float(3.141593)
  ["regexp"] =>
  regexp(/regexp/)
  ["array"] =>
  array(3) {
    [0] =>
    int(1)
    [1] =>
    int(2)
    [2] =>
    int(3)
  }
  ["object"] =>
  object(Object)#%d (1) {
    ["field"] =>
    string(3) "foo"
  }
  ["function"] =>
  object(Closure)#%d {
      function id(x) { return x; }
  }
  ["phpobject"] =>
  object(Foo)#%d (1) {
    ["$field"] =>
    string(3) "php"
  }
}
--- PHP var_dump of JS object ----
object(V8Object)#%d (12) {
  ["undefined"]=>
  NULL
  ["null"]=>
  NULL
  ["bool"]=>
  bool(true)
  ["string"]=>
  string(6) "string"
  ["uint"]=>
  int(1)
  ["int"]=>
  int(-1)
  ["number"]=>
  float(3.141592654)
  ["regexp"]=>
  object(V8Object)#%d (0) {
  }
  ["array"]=>
  array(3) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [2]=>
    int(3)
  }
  ["object"]=>
  object(V8Object)#%d (1) {
    ["field"]=>
    string(3) "foo"
  }
  ["function"]=>
  object(V8Function)#%d (0) {
  }
  ["phpobject"]=>
  object(Foo)#%d (1) {
    ["field"]=>
    string(3) "php"
  }
}
===EOF===
