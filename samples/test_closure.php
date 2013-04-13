<?php

$a = new V8Js();
$a->func = function ($a) { echo "Closure..\n"; };

try {
  $a->executeString("print(PHP.func); PHP.func(1);", "closure_test.js");
  $a->executeString("print(PHP.func); PHP.func(1);", "closure_test.js");
} catch (V8JsScriptException $e) {
  echo $e->getMessage(), "\n";
}
