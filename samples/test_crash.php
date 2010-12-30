<?php {
    $a = new V8Js();

    var_dump($a->executeString('Jst.write = function(s) { html += "EI TOIMI"; };' ."\n" .' Jst.evaluate("lol testi <%= 1 %>", {});'));
}
