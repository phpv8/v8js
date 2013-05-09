<?php {
    class Foo {
      private $v8 = NULL;

      public function __construct()
      {
		$this->v8 = new V8Js(null, array(), false);
		$this->v8->foo = $this;
//		$this->v8->executeString('asdasda< / sd', 'trycatch0');
//		$this->v8->executeString('blahnothere', 'trycatch1');
//	  	$this->v8->executeString('throw new SyntaxError();', 'throw');
//		$this->v8->executeString('function foo() {throw new SyntaxError();}', 'trycatch2');
//		$this->v8->executeString('try { foo(); } catch (e) { print(e + " catched by pure JS!\n"); }', 'trycatch3');
//		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print(e + " catched via PHP callback!\n"); }', 'trycatch4');
//		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print("catched!\n"); }', 'trycatch5');
//		$this->v8->executeString('try { PHP.foo.bar(); } catch (e) { print("catched!\n"); }', 'trycatch5');
		var_dump($this->v8->getPendingException());
      }

      public function bar()
      {
//		$this->v8->executeString('asdasda< / sd', 'trycatch0');
//		$this->v8->executeString('blahnothere', 'trycatch1');
      	$this->v8->executeString('throw new Error();', 'throw');
      }
    }
    
    try {
      $foo = new Foo();
    } catch (V8JsScriptException $e) {
      echo "PHP Exception: ", $e->getMessage(), "\n";
    }

}
