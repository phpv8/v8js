V8Js on MacOS
=============

Installation of V8Js on MacOS is pretty much straight forward.

If you have [brew](https://brew.sh) around, just `brew install
php70-v8js` (or `php71-v8js` / `php72-v8js` depending on your PHP
version) and you should be done. This will install a recent version
of V8 along with this extension.

Otherwise you need to compile latest v8 manually.

Compile latest v8
-----------------

```
cd /tmp

# Install depot_tools first (needed for source checkout)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# Download v8
fetch v8
cd v8

# (optional) If you'd like to build a certain version:
git checkout 3.32.6
gclient sync

# Compile V8 (using up to 8 CPU cores, requires a lot of RAM, adapt as needed)
make native library=shared -j8

# Install to /usr
mkdir -p /usr/local/lib /usr/local/include
cp out/native/lib*.dylib /usr/local/lib/
cp out/native/libv8_*.a  /usr/local/lib/
cp -R include/* /usr/local/include
```

You cannot install the libraries to any location you want since they
have a install name that is baked into the library.  You can check
the install name with `otool -D out/native/libv8.dylib`.

During the build snapshot generation may fail like so:

```
ACTION tools_gyp_v8_gyp_v8_snapshot_target_run_mksnapshot /Users/vagrant/v8/out/native/obj.target/v8_snapshot/geni/snapshot.cc
dyld: Library not loaded: /usr/local/lib/libicui18n.dylib
  Referenced from: /Users/vagrant/v8/out/native/mksnapshot
  Reason: image not found
/bin/sh: line 1: 18964 Trace/BPT trap: 5       "/Users/vagrant/v8/out/native/mksnapshot" --log-snapshot-positions --logfile "/Users/vagrant/v8/out/native/obj.target/v8_snapshot/geni/snapshot.log" --random-seed 314159265 "/Users/vagrant/v8/out/native/obj.target/v8_snapshot/geni/snapshot.cc"
make[1]: *** [/Users/vagrant/v8/out/native/obj.target/v8_snapshot/geni/snapshot.cc] Error 133
make: *** [native] Error 2
```

... if that happens, just copy libicu*.dylib to /usr/local/lib already
and start make again (then simply continue with installation):

```
cp out/native/libicu*.dylib /usr/local/lib/
make native library=shared -j8
```


Compile php-v8js itself
-----------------------

If you're using Apple LLVM compiler (instead of gcc) you need to pass the `-Wno-c++11-narrowing`
flag.  Otherwise compilation fails due to narrowing errors in PHP itself, which LLVM is much pickier
with (compared to gcc).

```
cd /tmp
git clone https://github.com/phpv8/v8js.git
cd v8js
phpize
./configure CXXFLAGS="-Wno-c++11-narrowing"
make
make test
make install
```

V8Js' build system assumes that the `icudtl.dat` file is located next to the `libv8.dylib`
library file and compiles the path into the library itself.  If for whatever reason the
`icudtl.dat` file is stored at a different place during runtime, you need to set the
php.ini variable `v8js.icudtl_dat_path` to point to the file.  Otherwise locale-aware
features of V8 will not work as expected.
