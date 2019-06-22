V8Js on Windows
===============

The V8Js PHP extension is primarily targeted at Unix platforms, especially
GNU/Linux.  However it is possible (and supported) to build on Windows, using
Microsoft Visual Studio.


**These instructions are pretty dated, and Windows is not currently a officially supported target of php-v8js.**
If you would like to make it work again, feel free to do so and provide Pull Requests as you see fit.


Building with MinGW or Cygwin is not officially supported; mainly since
Google v8 does not support builds on Cygwin (and building it on Cygwin is
currently broken).

Compared to installation on GNU/Linux it's way more tedious to install V8Js
on Windows, since you need to compile PHP with all its dependencies beforehand.

You need to have Visual Studio 2015 (aka VC14), make sure to install the C++
compiler (which is not installed by default any longer).

Compiling V8 using GN (required for 5.2 and higher)
---------------------------------------------------

[Build transcript using GN](https://github.com/phpv8/v8js/issues/287#issuecomment-284222529) by @Jan-E

This is how I built V8 5.8.

TL;DR: these are the essential commands for x64.

```
call git checkout 5.8-lkgr
call gclient sync
call gn gen v5.8\x64.release --args="is_component_build=true is_debug=false v8_use_snapshot=false enable_nacl=false target_cpu=\"x64\"" --ide=vs
cd v5.8\x64.release
msbuild all.sln
```

Prerequisites:

- VC14 aka VS2015 with the C and C++ compilers. The Community edition suffices.
- The Windows 10 Debugging Tools. If you did not install the Windows 10 SDK together with VC14, you can do a separate download from https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit => Windows 10 (WinDbg)

My initial setup was a mixture of the Linux & Windows commands at https://www.chromium.org/developers/how-tos/install-depot-tools

```
D:
cd \
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git chromium
set path=D:\chromium;%path%
set GYP_MSVS_VERSION=2015
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set GYP_CHROMIUM_NO_ACTION=0
cd chromium
fetch v8
git stash
```
Note: do not add the checkout dir to your path permanently if you've already installed git.exe somewhere in your path: the depot tools will create a git.bat in the checkout dir!

Then I ran the following batch script, inside a 'VS2015 x64 Native Tools Command Prompt' (x64) or a 'VS2015 x86 Native Tools Command Prompt' (x86):

```
D:
cd \chromium\v8
set WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\
set WindowsSDK_ExecutablePath_x64=C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.6.2 Tools\x64\
set WindowsSDK_ExecutablePath_x86=C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.6.2 Tools\
set path=D:\chromium;%path%
set GYP_MSVS_VERSION=2015
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set GYP_CHROMIUM_NO_ACTION=0
set xn=x86
if not %platform%x==x set xn=%platform%
if %xn%==X64 set xn=x64
call git checkout 5.8-lkgr
call git pull
call gclient sync
call gn gen v5.8\%xn%.release --args="is_component_build=true is_debug=false v8_use_snapshot=false enable_nacl=false target_cpu=\"%xn%\"" --ide=vs
cd v5.8\%xn%.release
msbuild all.sln /t:clean
msbuild all.sln
pause
unittests
```

Compiling V8 5.1 (and before)
-----------------------------

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
`v8*.dll` files from `\v8\build\Release` folder to
`\php-sdk\phpdev\vc14\x86\php-7.0.1\Release_TS\` first.

Then run

```
Release_TS\php.exe -d extension=php_v8js.dll -d extension_dir=Release_TS\
```

Alternatively copy all stuff to c:\php\ (including the DLL files from v8 build).

V8 library itself needs to load the `icudtl.dat` file at runtime.  Make sure php.ini
variable `v8js.icudtl_dat_path` points to this file; or as an alternative set
`PHP_V8_EXEC_PATH` in config.w32 to point to the directory where the dll and data file
are located.
