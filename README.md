V8Js
====

[![Build Status](https://travis-ci.org/phpv8/v8js.svg?branch=php7)](https://travis-ci.org/phpv8/v8js)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/gr8fkq8c08s06h31/branch/php7?svg=true)](https://ci.appveyor.com/project/stesie/v8js)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/phpv8/v8js/master/LICENSE)
[![Join the chat at https://gitter.im/phpv8/v8js](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/phpv8/v8js?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

V8Js is a PHP extension for Google's V8 Javascript engine.

The extension allows you to execute Javascript code in a secure sandbox from PHP. The executed code
can be restricted using a time limit and/or memory limit. This provides the possibility to execute
untrusted code with confidence.

If you execute untrusted JavaScript code, make sure to **use V8 version 6.4.388.18, 6.5.143 or higher**.
See notes on [untrusted code mitigations](https://github.com/v8/v8/wiki/Untrusted-code-mitigations)
for further details.


Minimum requirements
--------------------

- V8 Javascript Engine library (libv8) master <https://github.com/v8/v8-git-mirror> (trunk)

	V8 is Google's open source Javascript engine.
	V8 is written in C++ and is used in Google Chrome, the open source browser from Google.
	V8 implements ECMAScript as specified in ECMA-262, 5th edition.

	This extension requires V8 4.6.76 or higher.

    V8 releases are published rather quickly and the V8 team usually provides security support
    for the version line shipped with the Chrome browser (stable channel) and newer (only).
    For a version overview see https://omahaproxy.appspot.com/.

- PHP 7.0.0+

  This embedded implementation of the V8 engine uses thread locking so it works with ZTS enabled.


Pre-built binaries
------------------

For some very first steps, instead of compiling manually you might want to try out the [V8Js docker
image](https://registry.hub.docker.com/u/stesie/v8js/).  It has v8, v8js and php-cli pre-installed
so you can give it a try with PHP in "interactive mode".  There is no Apache, etc. running however.

For Ubuntu @pinepain has PPAs for [various versions of V8](https://launchpad.net/~pinepain).
You might want to install these and compile V8Js based on them.

There also is a Heroku extension pack that has recent V8Js versions for PHP 7.x, see
[@stesie's blog post](https://stesie.github.io/2016/03/heroku-custom-platform-repo) for details.

64-bit binaries for Windows [are made available by Jan-E here](https://www.apachelounge.com/viewtopic.php?t=6359).


Compiling latest version
------------------------

Building on Microsoft Windows is a bit more involved, see README.Win32.md file
for a quick run through.  Building on GNU/Linux and MacOS X is straight forward,
see README.Linux.md and README.MacOS.md files for a walk through with platform
specific notes.


PHP API
=======

```php
<?php
class V8Js
{
    /* Constants */

    const V8_VERSION = '';

    const FLAG_NONE = 1;
    const FLAG_FORCE_ARRAY = 2;
    const FLAG_PROPAGATE_PHP_EXCEPTIONS = 4;

    /* Methods */

    /**
     * Initializes and starts V8 engine and returns new V8Js object with it's own V8 context.
     * Snapshots are supported by V8 4.3.7 and higher.
     * @param string $object_name
     * @param array $variables
     * @param array $extensions
     * @param bool $report_uncaught_exceptions
     * @param string $snapshot_blob
     */
    public function __construct($object_name = "PHP", array $variables = [], array $extensions = [], $report_uncaught_exceptions = TRUE, $snapshot_blob = NULL)
    {}

    /**
     * Provide a function or method to be used to load required modules. This can be any valid PHP callable.
     * The loader function will receive the normalised module path and should return Javascript code to be executed.
     * @param callable $loader
     */
    public function setModuleLoader(callable $loader)
    {}

    /**
     * Provide a function or method to be used to normalise module paths. This can be any valid PHP callable.
     * This can be used in combination with setModuleLoader to influence normalisation of the module path (which
     * is normally done by V8Js itself but can be overriden this way).
     * The normaliser function will receive the base path of the current module (if any; otherwise an empty string)
     * and the literate string provided to the require method and should return an array of two strings (the new
     * module base path as well as the normalised name).  Both are joined by a '/' and then passed on to the
     * module loader (unless the module was cached before).
     * @param callable $normaliser
     */
    public function setModuleNormaliser(callable $normaliser)
    {}

    /**
     * Compiles and executes script in object's context with optional identifier string.
     * A time limit (milliseconds) and/or memory limit (bytes) can be provided to restrict execution. These options will throw a V8JsTimeLimitException or V8JsMemoryLimitException.
     * @param string $script
     * @param string $identifier
     * @param int $flags
     * @param int $time_limit in milliseconds
     * @param int $memory_limit in bytes
     * @return mixed
     */
    public function executeString($script, $identifier = '', $flags = V8Js::FLAG_NONE, $time_limit = 0, $memory_limit = 0)
    {}

    /**
     * Compiles a script in object's context with optional identifier string.
     * @param $script
     * @param string $identifier
     * @return resource
     */
    public function compileString($script, $identifier = '')
    {}

    /**
     * Executes a precompiled script in object's context.
     * A time limit (milliseconds) and/or memory limit (bytes) can be provided to restrict execution. These options will throw a V8JsTimeLimitException or V8JsMemoryLimitException.
     * @param resource $script
     * @param int $flags
     * @param int $time_limit
     * @param int $memory_limit
     */
    public function executeScript($script, $flags = V8Js::FLAG_NONE, $time_limit = 0 , $memory_limit = 0)
    {}

    /**
     * Set the time limit (in milliseconds) for this V8Js object
     * works similar to the set_time_limit php
     * @param int $limit
     */
    public function setTimeLimit($limit)
    {}

    /**
     * Set the memory limit (in bytes) for this V8Js object
     * @param int $limit
     */
    public function setMemoryLimit($limit)
    {}

    /**
     * Set the average object size (in bytes) for this V8Js object.
     * V8's "amount of external memory" is adjusted by this value for every exported object.  V8 triggers a garbage collection once this totals to 192 MB.
     * @param int $average_object_size
     */
    public function setAverageObjectSize($average_object_size)
    {}

    /**
     * Returns uncaught pending exception or null if there is no pending exception.
     * @return V8JsScriptException|null
     */
    public function getPendingException()
    {}

    /**
     * Clears the uncaught pending exception
     */
    public function clearPendingException()
    {}

    /** Static methods **/

    /**
     * Registers persistent context independent global Javascript extension.
     * NOTE! These extensions exist until PHP is shutdown and they need to be registered before V8 is initialized.
     * For best performance V8 is initialized only once per process thus this call has to be done before any V8Js objects are created!
     * @param string $extension_name
     * @param string $code
     * @param array $dependencies
     * @param bool $auto_enable
     * @return bool
     */
    public static function registerExtension($extension_name, $code, array $dependencies, $auto_enable = FALSE)
    {}

    /**
     * Returns extensions successfully registered with V8Js::registerExtension().
     * @return array|string[]
     */
    public static function getExtensions()
    {}

    /**
     * Creates a custom V8 heap snapshot with the provided JavaScript source embedded.
     * Snapshots are supported by V8 4.3.7 and higher.  For older versions of V8 this
     * extension doesn't provide this method.
     * @param string $embed_source
     * @return string|false
     */
    public static function createSnapshot($embed_source)
    {}
}

final class V8JsScriptException extends Exception
{
    /**
     * @return string
     */
    final public function getJsFileName( ) {}

    /**
     * @return int
     */
    final public function getJsLineNumber( ) {}
    /**
     * @return int
     */
    final public function getJsStartColumn( ) {}
    /**
     * @return int
     */
    final public function getJsEndColumn( ) {}

    /**
     * @return string
     */
    final public function getJsSourceLine( ) {}
    /**
     * @return string
     */
    final public function getJsTrace( ) {}
}

final class V8JsTimeLimitException extends Exception
{
}

final class V8JsMemoryLimitException extends Exception
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

PHP and JavaScript data types don't match exactly.  This is of course both languages have
data types to handle numbers.  Yet PHP differentiates between integers and floating point
numbers contrary JavaScript only has a type `Number`, which is a IEEE 754 floating point
number.  In many cases this doesn't matter at all, when both languages can represent the
same number well.  However there are edge cases.

On 64-bit systems PHP allows integers to have 64 significant bits, JavaScripts number type
(i.e. IEEE 754) however has 52 bit mantissa only.  Hence some precission will be lost.  This
starts to matter if you pass around integer values with more then 15 accurate decimal digits.


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

Snapshots
=========

First of all snapshots are incompatible with extensions. So when you see

    #
    # Fatal error in ../src/snapshot/startup-serializer.cc, line 122
    # Check failed: !isolate->has_installed_extensions().
    #

you need to remove all extension registrations.

First of all [custom startup snapshots](https://v8project.blogspot.de/2015/09/custom-startup-snapshots.html)
is a feature provided by V8 itself, built on top of it's general heap snapshots feature.  The idea is that, since
it is quite common to load some JavaScript library prior to any actual work to be done, that this library code
is also baked into the heap snapshot.

This extension provides an easy way to create those customized snapshots.  In order to create such a snapshot
with a `fibonacci` function baked into it, just call `V8Js::createSnapshot` statically like this:

```php
    $snapshot = V8Js::createSnapshot('var fibonacci = n => n < 3 ? 1 : fibonacci(n - 1) + fibonacci(n - 2)');
```

Then persist the contents of `$snapshot` to whereever you like, e.g. the local file system or maybe Redis.

If you need to create a new V8Js instance, simply pass the snapshot as 5th argument to the V8Js constructor:

```php
    $jscript = new V8Js('php', array(), array(), true, $snapshot);
    echo $jscript->executeString('fibonacci(43)') . "\n";
```

Keep in mind, that the code to be included in the snapshot may not directly call any of the functions exported
from PHP, since they are added right *after* the snapshot code is run.

Exceptions
==========

If the JavaScript code throws (without catching), causes errors or doesn't
compile, `V8JsScriptException` exceptions are thrown unless the `V8Js` object
is constructed with `report_uncaught_exceptions` set `FALSE`.

PHP exceptions that occur due to calls from JavaScript code by default are
*not* re-thrown into JavaScript context but cause the JavaScript execution to
be stopped immediately and then are reported at the location calling the JS code.

This behaviour can be changed by setting the `FLAG_PROPAGATE_PHP_EXCEPTIONS`
flag.  If it is set, PHP exception (objects) are converted to JavaScript
objects obeying the above rules and re-thrown in JavaScript context.  If they
are not caught by JavaScript code the execution stops and a
`V8JsScriptException` is thrown, which has the original PHP exception accessible
via `getPrevious` method.
