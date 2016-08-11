--TEST--
Test V8::executeString() : Issue #250 (early free of array)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class TestObject {

    var $data;

    public function setData($data) {
        $this->data = $data;
    }

    public function getData() {
        return $this->data;
    }

}

$v8 = new V8Js("server");

$code = <<< EOT
    server.response.setData({"new": true});
EOT;

$v8->response = new TestObject();

try {
    $result = $v8->executeString($code, null, \V8Js::FLAG_FORCE_ARRAY | \V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
    var_dump($v8->response->getData());
} catch (V8JsException $e) {
    var_dump($e);
}

?>
===EOF===
--EXPECT--
array(1) {
  ["new"]=>
  bool(true)
}
===EOF===
