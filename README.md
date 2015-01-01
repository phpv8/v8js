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
    This extension makes use of V8 isolates to ensure separation between multiple V8Js instances and uses the new isolate-based mechanism to throw exceptions, hence the need for 3.24.10 or above.

    For a detailed overview of which V8 version V8Js can be successfully built against, see the
    [Jenkins V8Js build matrix](https://jenkins.brokenpipe.de/job/docker-v8js-matrix/).

- PHP 5.3.3+

  This embedded implementation of the V8 engine uses thread locking so it should work with ZTS enabled.
  However, this has not been tested yet.
  
COMPILING LATEST VERSION
========================

Instead of compiling manually you might want to pull from the [V8Js docker
repository](https://registry.hub.docker.com/u/stesie/v8js/).

You also might want to try the Debian & Ubuntu packages available from
the Jenkins site at https://jenkins.brokenpipe.de/

Building on Microsoft Windows is more complicated, see README.Win32.md file
for a quick run through.

Compile latest v8
-----------------

```
cd /tmp
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
git clone https://github.com/v8/v8.git
cd v8
make dependencies
make native library=shared -j8
sudo mkdir -p /usr/lib /usr/include
sudo cp out/native/lib.target/lib*.so /usr/lib/
echo -e "create /usr/lib/libv8_libplatform.a\naddlib out/native/obj.target/tools/gyp/libv8_libplatform.a\nsave\nend" | sudo ar -M
sudo cp -R include/* /usr/include

```

If you don't want to overwrite the system copy of v8, replace `/usr` in
the above commands with `/tmp/v8-install` and then add
`--with-v8js=/tmp/v8-install` to the php-v8js `./configure` command below.

`libv8_libplatform.a` should not be copied directly since it's a thin
archive, i.e. it contains only pointers to  the build objects, which
otherwise must not be deleted.  The simple mri-script converts the
thin archive to a normal archive.

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

```php
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

        // Set the time limit (in milliseconds) for this V8Js object works similar to the set_time_limit php function
        public setTimeLimit( int $limit )

        // Set the memory limit (in bytes) for this V8Js object
        public setMemoryLimit( int $limit )

        // Compiles a script in object's context with optional identifier string.
        public mixed V8Js::compileString( string $script [, string $identifier ])

        // Executes a precompiled script in object's context.
        // A time limit (milliseconds) and/or memory limit (bytes) can be provided to restrict execution. These options will throw a V8JsTimeLimitException or V8JsMemoryLimitException.
        public mixed V8Js::executeScript( resource $script [, int $flags = V8Js::FLAG_NONE [, int $time_limit = 0 [, int $memory_limit = 0]]])

        // Returns uncaught pending exception or null if there is no pending exception.
        public V8JsScriptException V8Js::getPendingException( )

        // Clears the uncaught pending exception
        public clearPendingException( )

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

    class V8JsException extends RuntimeException
    {
    }

    final class V8JsScriptException extends V8JsException
    {
        /* Properties */
        protected string JsFileName = NULL;
        protected int JsLineNumber = NULL;
        protected int JsStartColumn = NULL;
        protected int JsEndColumn = NULL;
        protected string JsSourceLine = NULL;
        protected string JsTrace = NULL;
        
        /* Methods */
        final public string getJsFileName( )
        final public int getJsLineNumber( )
        final public int getJsStartColumn( )
        final public int getJsEndColumn( )
        final public string getJsSourceLine( )
        final public string getJsTrace( )
    }

    final class V8JsTimeLimitException extends V8JsException
    {
    }

    final class V8JsMemoryLimitException extends V8JsException
    {
    }
```

Javascript API
==============

```js
    // Print a string.
    print(string);

    // Dump the contents of a variable.
    var_dump(value);

    // Terminate Javascript execution immediately.
    exit();

    // CommonJS Module support to require external code.
    // This makes use of the PHP module loader provided via V8Js::setModuleLoader (see PHP API above).
    require("path/to/module");
```

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

Mapping Rules
=============

Native Arrays
-------------

Despite the common name the concept of arrays is very different between PHP and JavaScript.  In JavaScript an array is a contiguous collection of elements indexed by integral numbers from zero on upwards.  In PHP arrays can be sparse, i.e. integral keys need not be contiguous and may even be negative.  Besides PHP arrays may not only use integral numbers as keys but also strings (so-called associative arrays).  Contrary JavaScript arrays allow for properties to be attached to arrays, which isn't supported by PHP.  Those properties are not part of the arrays collection, for example `Array.prototype.forEach` method doesn't "see" these.

Generally PHP arrays are mapped to JavaScript "native" arrays if this is possible, i.e. the PHP array uses contiguous numeric keys from zero on upwards.  Both associative and sparse arrays are mapped to JavaScript objects.  Those objects have a constructor also called "Array", but they are not native arrays and don't share the Array.prototype, hence they don't (directly) support the typical array functions like `join`, `forEach`, etc.
PHP arrays are immediately exported value by value without live binding.  This is if you change a value on JavaScript side or push further values onto the array, this change is *not* reflected on PHP side.

If JavaScript arrays are passed back to PHP the JavaScript array is always converted to a PHP array.  If the JavaScript array has (own) properties attached, these are also converted to keys of the PHP array.


Native Objects
--------------

PHP objects passed to JavaScript are mapped to native JavaScript objects which have a "virtual" constructor function with the name of the PHP object's class.  This constructor function can be used to create new instances of the PHP class as long as the PHP class doesn't have a non-public `__construct` method.
All public methods and properties are visible to JavaScript code and the properties are live-bound, i.e. if a property's value is changed by JavaScript code, the PHP object is also affected.

If a native JavaScript object is passed to PHP the JavaScript object is mapped to a PHP object of `V8Object` class.  This object has all properties the JavaScript object has and is fully mutable.  If a function is assigned to one of those properties, it's also callable by PHP code.
The `executeString` function can be configured to always map JavaScript objects to PHP arrays by setting the `V8Js::FLAG_FORCE_ARRAY` flag.  Then the standard array behaviour applies that values are not live-bound, i.e. if you change values of the resulting PHP array, the JavaScript object is *not* affected.


PHP Objects implementing ArrayAccess, Countable
-----------------------------------------------

The above rule that PHP objects are generally converted to JavaScript objects also applies to PHP objects of `ArrayObject` type or other classes, that implement both the `ArrayAccess` and the `Countable` interface -- even so they behave like PHP arrays.

This behaviour can be changed by enabling the php.ini flag `v8js.use_array_access`.  If set, objects of PHP classes that implement the aforementioned interfaces are converted to JavaScript Array-like objects.  This is by-index access of this object results in immediate calls to the `offsetGet` or `offsetSet` PHP methods (effectively this is live-binding of JavaScript against the PHP object).  Such an Array-esque object also supports calling every attached public method of the PHP object + methods of JavaScript's native Array.prototype methods (as long as they are not overloaded by PHP methods).
