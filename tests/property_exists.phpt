--TEST--
Test V8::executeString() : property_exists/isset/empty on wrapped JS objects
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$v8 = new V8Js();

$JS = <<< EOT
(function(exports) {
  // begin module code
  exports.hello = function() { return 'hello'; };
  exports.isnull = null;
  exports.isundefined = undefined;
  exports.isfalse = false;
  exports.iszero = 0;
  exports.isquotezero = '0';
  exports.isemptyarray = [];
  exports.isemptystring = '';
  exports.istrue = true;
  // end module code
  return exports;
})({})
EOT;

$exports = $v8->executeString($JS, 'basic.js');

echo "= isset() =\n";
echo "bogus:         "; var_dump(isset( $exports->bogus ));
echo "hello:         "; var_dump(isset( $exports->hello ));
echo "isnull:        "; var_dump(isset( $exports->isnull ));
echo "isundefined:   "; var_dump(isset( $exports->isundefined ));
echo "isfalse:       "; var_dump(isset( $exports->isfalse ));
echo "iszero:        "; var_dump(isset( $exports->iszero ));
echo "isquotezero:   "; var_dump(isset( $exports->isquotezero ));
echo "isemptyarray:  "; var_dump(isset( $exports->isemptyarray ));
echo "isemptystring: "; var_dump(isset( $exports->isemptystring ));
echo "istrue:        "; var_dump(isset( $exports->istrue ));
echo "\n";

echo "= empty() =\n";
echo "bogus:         "; var_dump(empty( $exports->bogus ));
echo "hello:         "; var_dump(empty( $exports->hello ));
echo "isnull:        "; var_dump(empty( $exports->isnull ));
echo "isundefined:   "; var_dump(empty( $exports->isundefined ));
echo "isfalse:       "; var_dump(empty( $exports->isfalse ));
echo "iszero:        "; var_dump(empty( $exports->iszero ));
echo "isquotezero:   "; var_dump(empty( $exports->isquotezero ));
echo "isemptyarray:  "; var_dump(empty( $exports->isemptyarray ));
echo "isemptystring: "; var_dump(empty( $exports->isemptystring ));
echo "istrue:        "; var_dump(empty( $exports->istrue ));
echo "\n";

echo "= property_exists() =\n";
echo "bogus:         "; var_dump(property_exists( $exports, 'bogus' ));
echo "hello:         "; var_dump(property_exists( $exports, 'hello' ));
echo "isnull:        "; var_dump(property_exists( $exports, 'isnull' ));
echo "isundefined:   "; var_dump(property_exists( $exports, 'isundefined' ));
echo "isfalse:       "; var_dump(property_exists( $exports, 'isfalse' ));
echo "iszero:        "; var_dump(property_exists( $exports, 'iszero' ));
echo "isquotezero:   "; var_dump(property_exists( $exports, 'isquotezero' ));
echo "isemptyarray:  "; var_dump(property_exists( $exports, 'isemptyarray' ));
echo "isemptystring: "; var_dump(property_exists( $exports, 'isemptystring' ));
echo "istrue:        "; var_dump(property_exists( $exports, 'istrue' ));
echo "\n";

?>
===EOF===
--EXPECT--
= isset() =
bogus:         bool(false)
hello:         bool(true)
isnull:        bool(false)
isundefined:   bool(false)
isfalse:       bool(true)
iszero:        bool(true)
isquotezero:   bool(true)
isemptyarray:  bool(true)
isemptystring: bool(true)
istrue:        bool(true)

= empty() =
bogus:         bool(true)
hello:         bool(false)
isnull:        bool(true)
isundefined:   bool(true)
isfalse:       bool(true)
iszero:        bool(true)
isquotezero:   bool(true)
isemptyarray:  bool(true)
isemptystring: bool(true)
istrue:        bool(false)

= property_exists() =
bogus:         bool(false)
hello:         bool(true)
isnull:        bool(true)
isundefined:   bool(true)
isfalse:       bool(true)
iszero:        bool(true)
isquotezero:   bool(true)
isemptyarray:  bool(true)
isemptystring: bool(true)
istrue:        bool(true)

===EOF===
