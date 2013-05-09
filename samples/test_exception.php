<?php {
    class Foo {
      private $v8 = NULL;

      public function __construct()
      {
		$this->v8 = new V8Js();
		$this->v8->foo = $this;
		var_dump($this->v8->executeString('throw 1; PHP.foo.bar();', 'trycatch1'));
		var_dump($this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print("catched!\n"); }', 'trycatch2'));
      }

      public function bar()
      {
      	echo "To Bar!\n";
      	var_dump($this->v8->executeString('throw new Error();', 'throw'));
      }
    }
    
    try {
      $foo = new Foo();
    } catch (V8JsScriptException $e) {
      echo "PHP Exception: ", $e->getMessage(), "\n"; //var_dump($e);
    }

}
