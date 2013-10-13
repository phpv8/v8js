<?php

$v8 = new V8Js();
$v8->startDebugAgent('LineProcessor', 9222, V8Js::DEBUG_AUTO_BREAK_ALWAYS);


$JS = <<< EOT
  print("Hello LineProcessor User!\\n");

  function processLine(foo) {
    return foo.toUpperCase();
  };
EOT;

$v8->executeString($JS, 'processor.js');

$fh = fopen('php://stdin', 'r');

while(($line = fgets($fh))) {
  echo $v8->executeString('processLine('.json_encode($line).');');
}

