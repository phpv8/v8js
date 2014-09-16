V8Js
====

V8Js is a PHP extension for Google's V8 Javascript engine.

The extension allows you to execute Javascript code in a secure sandbox from PHP. The executed code can be restricted using a time limit and/or memory limit. This provides the possibility to execute untrusted code with confidence.


Minimum requirements
--------------------

- V8 Javascript Engine library (libv8) master <https://github.com/v8/v8/> (trunk)

	V8 is Google's open source Javascript engine.
	V8 is written in C++ and is used in Google Chrome, the open source browser from Google.
	V8 implements ECMAScript as specified in ECMA-262, 5th edition.
    This extension makes use of V8 isolates to ensure separation between multiple V8Js instances and uses the new isolate-based mechanism to throw exceptions, hence the need for 3.22.3 or above.

    For a detailed overview of which V8 version V8Js can be successfully built against, see the [Jenkins V8Js job list](http://jenkins.brokenpipe.de/view/v8js-with-v8-versions/).

- PHP 5.3.3+

  This embedded implementation of the V8 engine uses thread locking so it should work with ZTS enabled.
  However, this has not been tested yet.
  
COMPILING LATEST VERSION
========================

Compile latest v8
-----------------

```
cd /tmp
git clone https://github.com/v8/v8.git
cd v8
make dependencies
make native library=shared -j8
sudo mkdir -p /usr/lib /usr/include
sudo cp out/native/lib.target/lib*.so /usr/lib/
sudo cp include/v8* /usr/include

```

`v8` doesn't support `g++` version 4.8 (yet; see
<https://code.google.com/p/v8/issues/detail?id=2149>).  If your default `g++`
is version 4.8 you may need to install `g++` 4.7 and replace the `make`
command above with:
```
CXX=g++-4.7 LINK=g++-4.7 make native library=shared -j8
```

If you don't want to overwrite the system copy of v8, replace `/usr` in
the above commands with `/tmp/v8-install` and then add
`--with-v8js=/tmp/v8-install` to the php-v8js `./configure` command below.

Compile php-v8js itself
-----------------------

```
cd /tmp
git clone https://github.com/preillyme/v8js.git
cd v8js
phpize
./configure
make
sudo make install
```


PHP API
=======

    class V8Js
    {
        /* Constants */

        const string V8_VERSION;

        const int FLAG_NONE;
        const int FLAG_FORCE_ARRAY;

        const int DEBUG_AUTO_BREAK_NEVER;
        const int DEBUG_AUTO_BREAK_ONCE;
        const int DEBUG_AUTO_BREAK_ALWAYS;
    
        /* Methods */

        // Initializes and starts V8 engine and Returns new V8Js object with it's own V8 context.
        public __construct ( [ string $object_name = "PHP" [, array $variables = NULL [, array $extensions = NULL [, bool $report_uncaught_exceptions = TRUE ] ] ] )

        // Provide a function or method to be used to load required modules. This can be any valid PHP callable.
        // The loader function will receive the normalised module path and should return Javascript code to be executed.
        public setModuleLoader ( callable $loader )

        // Compiles and executes script in object's context with optional identifier string.
        // A time limit (milliseconds) and/or memory limit (bytes) can be provided to restrict execution. These options will throw a V8JsTimeLimitException or V8JsMemoryLimitException.
        public mixed V8Js::executeString( string $script [, string $identifier [, int $flags = V8Js::FLAG_NONE [, int $time_limit = 0 [, int $memory_limit = 0]]]])

        // Compiles a script in object's context with optional identifier string.
        public mixed V8Js::compileString( string $script [, string $identifier ])

        // Executes a precompiled script in object's context.
        // A time limit (milliseconds) and/or memory limit (bytes) can be provided to restrict execution. These options will throw a V8JsTimeLimitException or V8JsMemoryLimitException.
        public mixed V8Js::executeScript( resource $script [, int $flags = V8Js::FLAG_NONE [, int $time_limit = 0 [, int $memory_limit = 0]]])

        // Returns uncaught pending exception or null if there is no pending exception.
        public V8JsScriptException V8Js::getPendingException( )

        // Starts V8 debug agent for use with Google Chrome Developer Tools (Eclipse Plugin)
        public bool startDebugAgent( [ string $agent_name = "V8Js" [, $port = 9222 [, $auto_break = V8Js::DEBUG_AUTO_BREAK_NEVER ] ] ] )

        /** Static methods **/

        // Registers persistent context independent global Javascript extension.
        // NOTE! These extensions exist until PHP is shutdown and they need to be registered before V8 is initialized. 
        // For best performance V8 is initialized only once per process thus this call has to be done before any V8Js objects are created!
        public static bool V8Js::registerExtension( string $extension_name, string $code [, array $dependenciess [, bool $auto_enable = FALSE ] ] )

        // Returns extensions successfully registered with V8Js::registerExtension().
        public static array V8Js::getExtensions( )
    }

    final class V8JsScriptException extends Exception
    {
        /* Properties */
        protected string JsFileName = NULL;
        protected int JsLineNumber = NULL;
        protected string JsSourceLine = NULL;
        protected string JsTrace = NULL;
        
        /* Methods */
        final public string getJsFileName( )
        final public int getJsLineNumber( )
        final public string getJsSourceLine( )
        final public string getJsTrace( )
    }

    final class V8JsTimeLimitException extends Exception
    {
    }

    final class V8JsMemoryLimitException extends Exception
    {
    }

Javascript API
==============

    // Print a string.
    print(string);

    // Dump the contents of a variable.
    var_dump(value);

    // Terminate Javascript execution immediately.
    exit();

    // CommonJS Module support to require external code.
    // This makes use of the PHP module loader provided via V8Js::setModuleLoader (see PHP API above).
    require("path/to/module");

The JavaScript `in` operator, when applied to a wrapped PHP object,
works the same as the PHP `isset()` function.  Similarly, when applied
to a wrapped PHP object, JavaScript `delete` works like PHP `unset`.

```php
<?php
class Foo {
  var $bar = null;
}
$v8 = new V8Js();
$v8->foo = new Foo;
// This prints "no"
$v8->executeString('print( "bar" in PHP.foo ? "yes" : "no" );');
?>
```

PHP has separate namespaces for properties and methods, while JavaScript
has just one.  Usually this isn't an issue, but if you need to you can use
a leading `$` to specify a property, or `__call` to specifically invoke a
method.

```php
<?php
class Foo {
	var $bar = "bar";
	function bar($what) { echo "I'm a ", $what, "!\n"; }
}

$foo = new Foo;
// This prints 'bar'
echo $foo->bar, "\n";
// This prints "I'm a function!"
$foo->bar("function");

$v8 = new V8Js();
$v8->foo = new Foo;
// This prints 'bar'
$v8->executeString('print(PHP.foo.$bar, "\n");');
// This prints "I'm a function!"
$v8->executeString('PHP.foo.__call("bar", ["function"]);');
?>
```
