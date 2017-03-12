--TEST--
Test V8::executeString() : DOM object passed from PHP
--SKIPIF--
<?php
if(!class_exists('DomDocument')) {
  die('SKIP ext-dom required');
}
require_once(dirname(__FILE__) . '/skipif.inc');
?>
--FILE--
<?php

$JS = <<< EOT
print('js1: ', PHP.test.length, "\\n");
var elements = PHP.dom.getElementsByTagName('node');
print('js2: ', elements.length, "\\n");
var node = elements.item(0);
print("hasChildNodes: "); var_dump(node.hasChildNodes());
print("hasAttribute('class'): "); var_dump(node.hasAttribute('class'));
//var_dump(node);
EOT;

$dom = new DomDocument();
$dom->loadXML('<node class="test"/>');

$elements = $dom->getElementsByTagName('node');
echo 'php: ', $elements->length, "\n";
$node = $elements->item(0);
echo "hasChildNodes: "; var_dump($node->hasChildNodes());
echo "hasAttribute('class'): "; var_dump($node->hasAttribute('class'));
//var_dump($node);

$a = new V8Js();
$a->dom = $dom;
$a->test = array( 'length' => 1 );
$a->executeString($JS, "test.js");

?>
===EOF===
--EXPECT--
php: 1
hasChildNodes: bool(false)
hasAttribute('class'): bool(true)
js1: 1
js2: 1
hasChildNodes: bool(false)
hasAttribute('class'): bool(true)
===EOF===
