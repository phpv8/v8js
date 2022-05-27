--TEST--
Test V8::executeString() : Issue #250 (early free of array)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

class TestObject {

    private $data = [];
    private $meta = [];

    public function setTitle($title) {
        $this->a->b->title = $title;
    }

    public function getData() {
        return $this->data;
    }

    public function getMeta() {
        return $this->meta;
    }

    public function setData($data=[]) {
        $this->data = $data;
    }

    public function setMeta($meta) {
        return;
    }
}

$v8 = new V8Js("server");
$code = <<< EOT
    var v1 = server.response.getData();
    var v2 = server.response.getMeta();

    server.response.setData({});
    server.response.setTitle("ouch");
    server.response.setMeta({});
EOT;

$response = new TestObject();

$v8->response = $response;

try {
    $result = $v8->executeString($code);
    var_dump($v8->response);
} catch (V8JsException $e) {
    var_dump($e);
}

?>
===EOF===
--EXPECTF--
Fatal error: Uncaught Error: Attempt to modify property "b" on null in %s%eissue_250_001.php:9
Stack trace:
#0 [internal function]: TestObject->setTitle('ouch')
#1 %s%eissue_250_001.php(44): V8Js->executeString('    var v1 = se...')
#2 {main}
  thrown in %s%eissue_250_001.php on line 9
