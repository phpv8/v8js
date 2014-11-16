V8Js on Windows
===============

The V8Js PHP extension is primarily targeted at Unix platforms, especially
GNU/Linux.  However it is possible (and supported) to build on Windows, using
Microsoft Visual Studio.

Building with MinGW or Cygwin is not officially supported; mainly since
Google v8 does not support builds on Cygwin (and building it on Cygwin is
currently broken).

Compared to installation on GNU/Linux it's way more tedious to install V8Js
on Windows, since you need to compile PHP with all its dependencies beforehand.

The problem is that Google v8 requires (at least) Visual Studio 2013 as it
uses certain C++11 features not available in Visual Studio 2012.

Unfortunately the [PHP for Windows](http://windows.php.net/) project still
relies on either Visual Studio 2012 or Visual Studio 2008.
It supplies pre-compiled binary archives of required dependencies, however also
compiled with Visual Studio 2008 and 2012 only.

Since it is not compatible to link a v8 compiled with Visual Studio 2013 with
a PHP interpreter compiled with Visual Studio 2012, you need to step up and
compile PHP with Visual Studio 2013.  This requires to compile dependencies as
well, if you would like to use certain extensions or e.g. the Apache SAPI.


Compiling v8
------------

The Google v8 project already has excellent step-by-step guide on
[how to build with gyp](https://code.google.com/p/v8-wiki/wiki/BuildingWithGYP).

As a short run through:

* Download and install Python (make sure it adds python.exe to PATH during install)
  from http://www.python.org/download/
* Install Git from https://github.com/msysgit/msysgit/releases/download/Git-1.9.4-preview20140929/Git-1.9.4-preview20140929.exe
* Install Subversion from http://sourceforge.net/projects/win32svn/files/latest/download

Then open a command prompt

```
cd \
git clone https://github.com/v8/v8.git
cd v8
svn co http://gyp.googlecode.com/svn/trunk build/gyp
svn co https://src.chromium.org/chrome/trunk/deps/third_party/icu46 third_party/icu
svn co http://src.chromium.org/svn/trunk/tools/third_party/python_26@89111 third_party/python_26
svn co http://src.chromium.org/svn/trunk/deps/third_party/cygwin@66844 third_party/cygwin
svn co http://googletest.googlecode.com/svn/trunk testing/gtest --revision 643
svn co http://googlemock.googlecode.com/svn/trunk testing/gmock --revision 410

python build\gyp_v8 -Dcomponent=shared_library
"\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\devenv.com" /build Release build/All.sln
```

(alternatively `start build/all.sln` ... and build within the Visual Studio GUI)



Compiling PHP
-------------

There [Build your own PHP on Windows](https://wiki.php.net/internals/windows/stepbystepbuild)
guide on the PHP wiki.  However it concentrates on Visual Studio 2008 and 2012.
Since you need to use Visual Studio 2013 it doesn't apply very well.

This document concentrates on building V8Js for CLI SAPI (only).  In order
to enable more extensions you need to provide further dependencies, which may
be more or less cumbersome to build with Visual Studio beforehand.

A quick run through:

* install 7Zip from http://downloads.sourceforge.net/sevenzip/7z920-x64.msi
* create directory \php-sdk
* Download http://windows.php.net/downloads/php-sdk/php-sdk-binary-tools-20110915.zip
* ... and unpack to \php-sdk

Open "VS2013 x86 Native Tools Command Prompt"

```
cd \php-sdk
bin\phpsdk_setvars.bat
bin\phpsdk_buildtree.bat phpdev

mkdir vc12
mkdir vc12\x86
mkdir vc12\x86\deps
mkdir vc12\x86\deps\bin
mkdir vc12\x86\deps\include
mkdir vc12\x86\deps\lib
```

* download PHP from http://php.net/get/php-5.5.18.tar.gz/from/a/mirror
  and unpack to below `\php-sdk\phpdev\vc12\x86`
* from `\v8\build\Release\lib` copy `icui18n.lib`, `icuuc.lib` and `v8.lib`
  to deps\lib folder
* from `\v8\include copy` all v8*.h files to deps\include folder
* within the PHP source code folder create a sub-directory named `pecl`
* download V8Js and unpack it into a seperate directory below the `pecl` folder

(still in "VS2013 x86 Native Tools Command Prompt")

```
cd \php-sdk\phpdev\vc12\x86\php-5.5.18\

buildconf
configure --disable-all --enable-cli --with-v8js
nmake
```

After nmake completes the php.exe is in Release_TS\ directory.

In order to try things out in place, copy over `icui18n.dll`, `icuuc.dll` and
`v8.dll` file from `\v8\build\Release` folder to
`\php-sdk\phpdev\vc12\x86\php-5.5.18\Release_TS\` first.

Then run

```
php.exe -d extension=php_v8js.dll -d extension_dir=\php-sdk\phpdev\vc12\x86\php-5.5.18\Release_TS\
```

Alternatively copy all stuff to c:\php\ (including the three DLL files from
v8 build).
