<?php

$a = new V8Js();

try {
	var_dump($a->executeString("date = new Date('September 8, 1975 09:00:00'); print(date + '\\n'); date;", "test.js"));
} catch (V8JsScriptException $e) {
	echo $e->getMessage(), "\n";
}
