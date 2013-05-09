<?php
/*
$v8 = new V8Js;
$v8->func = function ($a) { return var_export(func_get_args(), TRUE); };
$v8->executeString("PHP.func();", "arg_test1.js");
exit;
*/

var_dump(V8Js::registerExtension('myparser.js', 'function foo() { print("foobar!\n"}', array('jstparser.js', 'json-template.js'), false));
var_dump(V8Js::registerExtension('myparser.js', 'function foo() { print("foobar!\n"}', array('jstparser.js', 'json-template.js'), false));
var_dump(V8Js::registerExtension('jstparser.js', file_get_contents('js/jstparser.js'), array(), false));
//V8Js::registerExtension('json-template.js', file_get_contents('js/json-template.js'), array(), false);

var_dump(V8JS::getExtensions());

$a = new V8Js('myobj', array(), array('jstparser.js'));

$jstparser = <<< 'EOT'
var template = 'Gold &amp; Hot Rod Red, as seen in the new <a href="http://blog.markturansky.com/archives/51">Iron Man trailer</a>!' + "\n" +
'<table cellspacing="0" cellpadding="4">' + "\n" +
'    <% for(var i = 0; i < 10; i++){ %> ' + "\n" +
'        <tr>' + "\n" +
'        <td style="background-color: <%= i % 2 == 0 ? \'red\' : \'gold\' %>">' + "\n" +
'            Hi, <%=name%>! i is <%= i %>' + "\n" +
'        </td>' + "\n" +
'        </tr>' + "\n" +
'    <% } %>' + "\n" +
'</table>' + "\n" +
'Note that name is HTML escaped by default. Here it is without escaping:'+
'<%+ name %>';
Jst.evaluateSingleShot(template, {"name":"foobar"});
EOT;

echo($a->executeString($jstparser, "ext_test1.js")), "\n";

$a->_SERVER = $_SERVER;
$a->func = function ($a) { echo "Closure..\n"; };

$a->executeString("print(myobj._SERVER['HOSTNAME']);", "test1.js");
$a->executeString("print(myobj.func); myobj.func(1);", "closure_test.js");

$JS = <<<'EOT'
function dump(a)
{
	for (var i in a) { 
	  var val = a[i];
	  print(i + ' => ' + val + "\n");
	}
}
function foo()
{
  var bar = 'bar';
  var foo = 'foo';
  return foo + bar;
}
function test()
{
  var a = 'PHP version: ' + PHP.phpver;
  phpver = 'changed in JS!';
  return a;
}
function loop()
{
  var foo = 'foo';
  while(true)
  {
    foo + 'bar';
  }
}
function output()
{
	while(true)
	{
		print("output:foo\n");
		sleep(5);
		exit();
	}
};
function simplearray()
{
	print(myarray.a + "\n");
	print(myarray.b + "\n");
	print(myarray.c + "\n");
	print(myarray.d + "\n");
}
function bigarray()
{
	print(PHP.$_SERVER['HOSTNAME'] + "\n");
	print(PHP.$_SERVER.argv + "\n");
}
EOT;

$jsontemplate = <<< EOT
var t = jsontemplate.Template("{# This is a comment and will be removed from the output.}{.section songs}<h2>Songs in '{playlist-name}'</h2><table width=\"100%\">{.repeated section @}<tr><td><a href=\"{url-base|htmltag}{url|htmltag}\">Play</a><td><i>{title}</i></td><td>{artist}</td></tr>{.end}</table>{.or}<p><em>(No page content matches)</em></p>{.end}");
t.expand({
"url-base": "http://example.com/music/", 
"playlist-name": "Epic Playlist", 
"songs": [
{
"url": "1.mp3", 
"artist": "Grayceon", 
"title": "Sounds Like Thunder"
}, 
{
"url": "2.mp3", 
"artist": "Thou", 
"title": "Their Hooves Carve Craters in the Earth"
}]});
EOT;

class tester
{
	public $foo = 'bar';
	private $my_private = 'arf';
	protected $my_protected = 'argh';

	function mytest() { echo 'Here be monsters..', "\n"; }
}

$a = new V8Js();
$a->obj = new tester();
$a->phpver = phpversion();
$a->argv = $_SERVER['argv'];
$a->integer = 1;
$a->float = 3.14;
$a->{'$'._SERVER} = $_SERVER;
$a->GLOBALS = $GLOBALS;
$a->myarray = array(
	'a' => 'Array value for key A',
	'b' => 'Array value for key B',
	'c' => 'Array value for key C',
	'd' => 'Array value for key D',
);
$a->arr = array("first", "second", "third");

$a->executeString($JS, "test1.js");
$a->executeString("bigarray()", "test1.js");

try {
  echo($a->executeString($jstparser, "test2.js")), "\n";
  var_dump($a->executeString($jsontemplate, "test1.js"));
} catch (V8JsScriptException $e) {
  echo $e->getMessage();
}

// Test for context handling

$a->executeString($JS, "test1.js");
$a->executeString("bigarray();");

echo '$a->obj: ', "\n"; $a->executeString("dump(PHP.obj);");
echo '$a->arr: ', "\n"; $a->executeString("dump(PHP.arr);");
echo '$a->argv: ', "\n"; $a->executeString("dump(PHP.argv);");

var_dump($a->argv);
var_dump($a->executeString("test();"));
var_dump($a->executeString("test();"));

$b = new V8Js();

var_dump($a->phpver, $a->executeString("test();"));

$b->executeString($JS, "test2.js");
var_dump($b->executeString("test();"));
var_dump($b->executeString("print('foobar\\n');"));

// Exception methods

try {
  $b->executeString("foobar; foo();", "extest.js");
} catch (V8JsScriptException $e) {
  var_dump($e, $e->getJsFileName(), $e->getJsLineNumber(), $e->getJsSourceLine(), $e->getJsTrace());
}
