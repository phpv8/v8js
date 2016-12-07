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

Compiling V8 using GN (required for 5.6 and higher)
---------------------------------------------------

[Build transcript using GN](https://github.com/phpv8/v8js/issues/272) by @Jan-E

TL;DR: up until 'msbuild all.sln' are the essential steps
Do not forget to build v8_libbase.lib & v8_libplatform.lib afterwards

```
cd obj\v8_libplatform && lib /out:v8_libplatform.lib *.obj
cd ..\\..\obj\v8_libbase && lib /out:v8_libbase.lib *.obj
```

My initial setup was a mixture of the Linux & Windows commands at
https://www.chromium.org/developers/how-tos/install-depot-tools
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

Transcript of building V8 v5.4.500.43 in a VS2015 x64 command prompt:
```
D:\chromium\v8>set path=D:\chromium;%path%

D:\chromium\v8>set GYP_MSVS_VERSION=2015

D:\chromium\v8>set DEPOT_TOOLS_WIN_TOOLCHAIN=0

D:\chromium\v8>set GYP_CHROMIUM_NO_ACTION=0

D:\chromium\v8>git checkout 5.4-lkgr
Checking out files: 100% (1561/1561), done.
Switched to branch '5.4-lkgr'
Your branch is up-to-date with 'origin/5.4-lkgr'.

D:\chromium\v8>gclient sync
Syncing projects:  45% (11/24) v8/third_party/instrumented_libraries

D:\chromium\v8>gclient sync
Syncing projects: 100% (24/24) v8/third_party/icu

v8/tools/mb (ERROR)
----------------------------------------
[0:00:32] Started.
[0:00:37] From https://chromium.googlesource.com/chromium/src/tools/mb
[0:00:37]    dc02dd5..b1e1230  master     -> origin/master
----------------------------------------
Error: 20>
20> ____ v8\tools\mb at 99788b8b516c44d7db25cfb68695bc234fdee5ed
20>     You have unstaged changes.
20>     Please commit, stash, or reset.

D:\chromium\v8>cd tools\mb

D:\chromium\v8\tools\mb>git stash
Saved working directory and index state WIP on (no branch): 99788b8 Fix libfuzzer configura
tions to actually use proprietary codecs.
HEAD is now at 99788b8 Fix libfuzzer configurations to actually use proprietary codecs.

D:\chromium\v8\tools\mb>cd ..\..

D:\chromium\v8>gclient sync
Syncing projects:  37% ( 9/24) v8/test/test262/data

After a lot or messages:

Hook ''D:\chromium\python276_bin\python.exe' v8/gypfiles/gyp_v8' took 11.56 secs

D:\chromium\v8>gn args v5.4/x64.release
Waiting for editor on "D:\chromium\v8\v5.4\x64.release\args.gn"...

In your editor change args.gn to something like

# Build arguments go here. Examples:
is_component_build = true
is_debug = false
# See "gn args <out_dir> --list" for available build arguments.
target_cpu="x64"
v8_target_cpu="x64"

And close your editor

In the VS2015 command prompt:

Generating files...
Done. Made 60 targets from 43 files in 2791ms

D:\chromium\v8>gn gen v5.4/x64.release --ide=vs
Generating Visual Studio projects took 268ms
Done. Made 60 targets from 43 files in 2374ms

D:\chromium\v8>cd v5.4\x64.release

D:\chromium\v8\v5.4\x64.release>msbuild all.sln
Microsoft (R) Build Engine version 14.0.25420.1
Copyright (C) Microsoft Corporation. All rights reserved.

Building the projects in this solution one at a time. To enable parallel build, please add
the "/m" switch.
Build started 24/11/16 21:40:52.
Project "D:\chromium\v8\v5.4\x64.release\all.sln" on node 1 (default targets).
ValidateSolutionConfiguration:
  Building solution configuration "GN|x64".
Project "D:\chromium\v8\v5.4\x64.release\all.sln" (1) is building "D:\chromium\v8\v5.4\x64
.release\obj\build\config\sanitizers\deps.vcxproj" (2) on node 1 (default targets).
Build:
  call ninja.exe -C ../../../../../../v5.4/x64.release/ obj/build/config/sanitizers/deps.s
  tamp
  ninja: Entering directory `../../../../../../v5.4/x64.release/'
  [1/2] STAMP obj/build/config/sanitizers/deps_no_options.stamp
  [2/2] STAMP obj/build/config/sanitizers/deps.stamp
  
etc. etc.

  [902/906] CXX obj/v8_external_snapshot/snapshot-external.obj
  [903/906] STAMP obj/v8_external_snapshot.stamp
  [904/906] STAMP obj/v8_maybe_snapshot.stamp
  [905/906] LINK(DLL) v8.dll v8.dll.lib v8.dll.pdb
LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]

  [906/906] LINK d8.exe d8.exe.pdb
LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]

Done Building Project "D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj" (default targets).

Project "D:\chromium\v8\v5.4\x64.release\all.sln" (1) is building "D:\chromium\v8\v5.4\x64.release\obj\fuzzer_support.vcxproj" (7) on node 1 (default targets).
Build:
  call ninja.exe -C ../../../v5.4/x64.release/ obj/fuzzer_support.stamp
  ninja: Entering directory `../../../v5.4/x64.release/'
  [1/3] STAMP obj/fuzzer_support.inputdeps.stamp
  [2/3] CXX obj/fuzzer_support/fuzzer-support.obj
  [3/3] STAMP obj/fuzzer_support.stamp
Done Building Project "D:\chromium\v8\v5.4\x64.release\obj\fuzzer_support.vcxproj" (default targets).

Project "D:\chromium\v8\v5.4\x64.release\all.sln" (1) is building "D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj" (8) on node 1 (default targets).
Build:
  call ninja.exe -C ../../../v5.4/x64.release/ obj/gn_all.stamp
  ninja: Entering directory `../../../v5.4/x64.release/'
  [1/333] STAMP obj/v8_hello_world.inputdeps.stamp
  [2/333] STAMP obj/json_fuzzer.inputdeps.stamp
  [3/333] STAMP obj/parser_fuzzer.inputdeps.stamp
  [4/333] STAMP obj/v8_parser_shell.inputdeps.stamp
  [5/333] STAMP obj/regexp_fuzzer.inputdeps.stamp
  
etc. etc.

Done Building Project "D:\chromium\v8\v5.4\x64.release\all.sln" (default targets).


Build succeeded.

"D:\chromium\v8\v5.4\x64.release\all.sln" (default target) (1) ->
"D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj" (default target) (6) ->
(Build target) ->
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\d8.vcxproj]


"D:\chromium\v8\v5.4\x64.release\all.sln" (default target) (1) ->
"D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj" (default target) (8) ->
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]
  LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/OPT:ICF' specification [D:\chromium\v8\v5.4\x64.release\obj\gn_all.vcxproj]

    16 Warning(s)
    0 Error(s)

Time Elapsed 00:41:27.06

D:\chromium\v8\v5.4\x64.release>type d:\batch\v8x64.bat
xcopy v8*.lib ..\win64build.vc14\lib\ /y
xcopy v8*.dll ..\win64build.vc14\bin\ /y
xcopy v8*.dll.pdb ..\win64build.vc14\bin\ /y
xcopy icu*.lib ..\win64build.vc14\lib\ /y
xcopy icu*.dll ..\win64build.vc14\bin\ /y
xcopy icu*.dll.pdb ..\win64build.vc14\bin\ /y
cd obj\v8_libplatform
lib /out:..\..\..\win64build.vc14\lib\v8_libplatform.lib *.obj
xcopy ..\v8_libplatform_cc.pdb ..\..\..\win64build.vc14\lib\ /y
cd ..\..\obj\v8_libbase
lib /out:..\..\..\win64build.vc14\lib\v8_libbase.lib *.obj
xcopy ..\v8_libbase_cc.pdb ..\..\..\win64build.vc14\lib\ /y
cd ..\..\
xcopy ..\..\include\*.h ..\win64build.vc14\include\ /s /y

D:\chromium\v8\v5.4\x64.release>d:\batch\v8x64.bat

D:\chromium\v8\v5.4\x64.release>xcopy v8*.lib ..\win64build.vc14\lib\ /y
D:v8.dll.lib
D:v8_parser_shell.lib
D:v8_simple_json_fuzzer.lib
D:v8_simple_parser_fuzzer.lib
D:v8_simple_regexp_fuzzer.lib
D:v8_simple_wasm_asmjs_fuzzer.lib
D:v8_simple_wasm_fuzzer.lib
7 File(s) copied

D:\chromium\v8\v5.4\x64.release>xcopy v8*.dll ..\win64build.vc14\bin\ /y
D:v8.dll
1 File(s) copied

D:\chromium\v8\v5.4\x64.release>xcopy v8*.dll.pdb ..\win64build.vc14\bin\ /y
D:v8.dll.pdb
1 File(s) copied

D:\chromium\v8\v5.4\x64.release>xcopy icu*.lib ..\win64build.vc14\lib\ /y
D:icui18n.dll.lib
D:icuuc.dll.lib
2 File(s) copied

D:\chromium\v8\v5.4\x64.release>xcopy icu*.dll ..\win64build.vc14\bin\ /y
D:icui18n.dll
D:icuuc.dll
2 File(s) copied

D:\chromium\v8\v5.4\x64.release>xcopy icu*.dll.pdb ..\win64build.vc14\bin\ /y
D:icui18n.dll.pdb
D:icuuc.dll.pdb
2 File(s) copied

D:\chromium\v8\v5.4\x64.release>cd obj\v8_libplatform

D:\chromium\v8\v5.4\x64.release\obj\v8_libplatform>lib /out:..\..\..\win64build.vc14\lib\v8
_libplatform.lib *.obj
Microsoft (R) Library Manager Version 14.00.24215.1
Copyright (C) Microsoft Corporation.  All rights reserved.


D:\chromium\v8\v5.4\x64.release\obj\v8_libplatform>xcopy ..\v8_libplatform_cc.pdb ..\..\..\
win64build.vc14\lib\ /y
..\v8_libplatform_cc.pdb
1 File(s) copied

D:\chromium\v8\v5.4\x64.release\obj\v8_libplatform>cd ..\..\obj\v8_libbase

D:\chromium\v8\v5.4\x64.release\obj\v8_libbase>lib /out:..\..\..\win64build.vc14\lib\v8_lib
base.lib *.obj
Microsoft (R) Library Manager Version 14.00.24215.1
Copyright (C) Microsoft Corporation.  All rights reserved.


D:\chromium\v8\v5.4\x64.release\obj\v8_libbase>xcopy ..\v8_libbase_cc.pdb ..\..\..\win64bui
ld.vc14\lib\ /y
..\v8_libbase_cc.pdb
1 File(s) copied

D:\chromium\v8\v5.4\x64.release\obj\v8_libbase>cd ..\..\

D:\chromium\v8\v5.4\x64.release>xcopy ..\..\include\*.h ..\win64build.vc14\include\ /s /y
..\..\include\v8-debug.h
..\..\include\v8-experimental.h
..\..\include\v8-platform.h
..\..\include\v8-profiler.h
..\..\include\v8-testing.h
..\..\include\v8-util.h
..\..\include\v8-version.h
..\..\include\v8.h
..\..\include\v8config.h
..\..\include\libplatform\libplatform.h
..\..\include\libplatform\v8-tracing.h
11 File(s) copied

D:\chromium\v8\v5.4\x64.release>type ..\..\include\v8-version.h
// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_INCLUDE_VERSION_H_  // V8_VERSION_H_ conflicts with src/version.h
#define V8_INCLUDE_VERSION_H_

// These macros define the version number for the current version.
// NOTE these macros are used by some of the tool scripts and the build
// system so their names cannot be changed without changing the scripts.
#define V8_MAJOR_VERSION 5
#define V8_MINOR_VERSION 4
#define V8_BUILD_NUMBER 500
#define V8_PATCH_LEVEL 43

// Use 1 for candidates and 0 otherwise.
// (Boolean macro values are not supported by all preprocessors.)
#define V8_IS_CANDIDATE_VERSION 0

#endif  // V8_INCLUDE_VERSION_H_
```

Compiling V8 5.5 (and before)
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
`v8.dll` file from `\v8\build\Release` folder to
`\php-sdk\phpdev\vc14\x86\php-7.0.1\Release_TS\` first.

Then run

```
Release_TS\php.exe -d extension=php_v8js.dll -d extension_dir=Release_TS\
```

Alternatively copy all stuff to c:\php\ (including the three DLL files from
v8 build).
