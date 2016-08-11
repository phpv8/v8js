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
--EXPECT--
Warning: Creating default object from empty value in /home/stesie/Projekte/php-7.0.9/ext/v8js/tests/issue_250_001.php on line 9
object(TestObject)#2 (3) {
  ["data":"TestObject":private]=>
  object(V8Object)#3 (0) {
  }
  ["meta":"TestObject":private]=>
  array(0) {
  }
  ["a"]=>
  object(stdClass)#4 (1) {
    ["b"]=>
    object(stdClass)#5 (1) {
      ["title"]=>
      string(4) "ouch"
    }
  }
}
===EOF===
