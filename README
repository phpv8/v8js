V8Js
====

This is a PHP extension for Google's V8 Javascript engine 


Minimum requirements
--------------------

- V8 library version 2.5.8 <http://code.google.com/p/v8/> (trunk)

- PHP 5.3.3+ (non-ZTS build preferred)
  Note: V8 engine is not natively thread safe and this extension
  has not been designed to work around it either yet and might or
  might not work properly with ZTS enabled PHP. :)


API
===

class V8Js
{
  /* Constants */

  const string V8_VERSION;
  const int FLAG_NONE;
  const int FLAG_FORCE_ARRAY;

  /* Methods */

  // Initializes and starts V8 engine and Returns new V8Js object with it's own V8 context.
  public __construct ( [string object_name = "PHP" [, array variables = NULL [, array extensions = NULL [, bool report_uncaught_exceptions = TRUE]]] )

  // Compiles and executes script in object's context with optional identifier string.
  public mixed V8Js::executeString( string script [, string identifier [, int flags = V8Js::FLAG_NONE]])

  // Returns uncaught pending exception or null if there is no pending exception.
  public V8JsException V8Js::getPendingException( void )

  /** Static methods **/

  // Registers persistent context independent global Javascript extension.
  // NOTE! These extensions exist until PHP is shutdown and they need to be registered before V8 is initialized. 
  // For best performance V8 is initialized only once per process thus this call has to be done before any V8Js objects are created!
  public static bool V8Js::registerExtension(string ext_name, string script [, array deps [, bool auto_enable = FALSE]])

  // Returns extensions successfully registered with V8Js::registerExtension().
  public static array V8Js::getExtensions( void )
}

final class V8JsException extends Exception
{
  /* Properties */
  protected string JsFileName = NULL;
  protected int JsLineNumber = NULL;
  protected string JsSourceLine = NULL;
  protected string JsTrace = NULL;

  /* Methods */
  final public string getJsFileName( void )
  final public int getJsLineNumber( void )
  final public string getJsSourceLine( void )
  final public string getJsTrace( void )
}
