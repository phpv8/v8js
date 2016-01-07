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

You need to have Visual Studio 2015 (aka VC14), make sure to install the C++
compiler (which is not installed by default any longer).

Compiling v8
------------

The Google v8 project already has excellent step-by-step guide on
[how to build with gyp](https://github.com/v8/v8/wiki/Building%20with%20Gyp).

As a short run through:

* Download and install Python (make sure it adds python.exe to PATH during install)
  from http://www.python.org/download/
* Install Git from https://github.com/msysgit/msysgit/releases/download/Git-1.9.4-preview20140929/Git-1.9.4-preview20140929.exe
* Install Subversion from http://sourceforge.net/projects/win32svn/files/latest/download

Then open a command prompt

```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git 
fetch v8
git checkout 4.7.80.25
gclient sync
python build\gyp_v8 -Dcomponent=shared_library -Dv8_use_snapshot=0
CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\vcvars32.bat"
msbuild build\All.sln /m /p:Configuration=Release
```

(alternatively `start build/all.sln` ... and build within the Visual Studio GUI)



Compiling PHP
-------------

Then follow the [build your own PHP on Windows](https://wiki.php.net/internals/windows/stepbystepbuild)
guide on the PHP wiki.  Use Visual Studio 2015 with PHP7.

A quick run through:

* install 7Zip from http://downloads.sourceforge.net/sevenzip/7z920-x64.msi
* create directory \php-sdk
* Download http://windows.php.net/downloads/php-sdk/php-sdk-binary-tools-20110915.zip
* ... and unpack to \php-sdk

Open "VS2015 x86 Native Tools Command Prompt"

```
cd \php-sdk
bin\phpsdk_setvars.bat
bin\phpsdk_buildtree.bat phpdev

mkdir vc14
mkdir vc14\x86
mkdir vc14\x86\deps
mkdir vc14\x86\deps\bin
mkdir vc14\x86\deps\include
mkdir vc14\x86\deps\lib
```

* download PHP from http://php.net/get/php-7.0.1.tar.gz/from/a/mirror
  and unpack to below `\php-sdk\phpdev\vc14\x86`
* from `\v8\build\Release\lib` copy `icui18n.lib`, `icuuc.lib`, `v8.lib`,
  `v8_libbase.lib` and `v8_libplatform.lib` to deps\lib folder
* from `\v8\include` copy all v8*.h files to deps\include folder
* copy `v8-platform.h` again to deps\include\include folder
* download V8Js and unpack it into a separate directory below `ext` folder
* make sure `config.w32` file inside that folder defines version of the compiled v8

(still in "VS2015 x86 Native Tools Command Prompt")

```
cd \php-sdk\phpdev\vc14\x86\php-7.0.1\

buildconf
configure --disable-all --enable-cli --with-v8js
nmake
```

After nmake completes the php.exe is in Release_TS\ directory.

In order to try things out in place, copy over `icui18n.dll`, `icuuc.dll` and
`v8.dll` file from `\v8\build\Release` folder to
`\php-sdk\phpdev\vc14\x86\php-7.0.1\Release_TS\` first.

Then run

```
Release_TS\php.exe -d extension=php_v8js.dll -d extension_dir=Release_TS\
```

Alternatively copy all stuff to c:\php\ (including the three DLL files from
v8 build).
