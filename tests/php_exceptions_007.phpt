--TEST--
Test V8::executeString() : PHP Exception handling (throwed inside magic method)
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
class SomeClass {
    function someMethod($someVariable) {
        return $someVariable;
    }

    public function triggerException() {
        throw new Exception("Some exception");
    }

    public function __get($key) {
        $this->triggerException();
    }
}

function execute($code, $flags = V8Js::FLAG_NONE) {
    $js = new V8Js();
    $js->output = new stdClass();
    $js->SomeClassInstance = new SomeClass();
    try {
        $js->executeString("
                try {
                    $code
                } catch(e) {
                    PHP.output.result = 'Caught exception at javascript level : ' + e.getMessage();
                }
            ", '', $flags);
        print($js->output->result.PHP_EOL);
    } catch (Exception $e) {
        print( "Caught exception at php level : ".$e->getMessage().PHP_EOL);
    }
}

execute("PHP.SomeClassInstance.triggerException();");
execute("PHP.SomeClassInstance.someMethod(PHP.SomeClassInstance.TriggerMagicMethod);");
execute("PHP.SomeClassInstance.TriggerMagicMethod;");
execute("PHP.SomeClassInstance.triggerException();", V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
execute("PHP.SomeClassInstance.someMethod(PHP.SomeClassInstance.TriggerMagicMethod);", V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
execute("PHP.SomeClassInstance.TriggerMagicMethod;", V8Js::FLAG_PROPAGATE_PHP_EXCEPTIONS);
?>
===EOF===
--EXPECTF--
Caught exception at php level : Some exception
Caught exception at php level : Some exception
Caught exception at php level : Some exception
Caught exception at javascript level : Some exception
Caught exception at javascript level : Some exception
Caught exception at javascript level : Some exception
===EOF===
