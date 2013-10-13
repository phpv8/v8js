<?php

class LineProcessor {
  protected $_processor;

  public function readLineLoop() {
	$fh = fopen('php://stdin', 'r');
	$p = $this->_processor;

	while(($line = fgets($fh))) {
	  echo $p($line);
	}
  }

  public function setProcessor($p) {
	$this->_processor = $p;
  }
}

$v8 = new V8Js();
$v8->lp = new LineProcessor();
$v8->startDebugAgent('LineProcessor', 9222, V8Js::DEBUG_AUTO_BREAK_NEVER);

$JS = <<< EOT
  print("Hello LineProcessor User!\\n");

  PHP.lp.setProcessor(function (foo) {
	return foo.toUpperCase();
  });

  PHP.lp.readLineLoop();
EOT;

$v8->executeString($JS, 'processor.js');


