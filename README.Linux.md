V8Js on GNU/Linux
=================

Installation of V8Js on GNU/Linux is pretty much straight forward.

The biggest hurdle actually is that you need a rather new V8 library.
However many distributions still ship the rusty version 3.14, published
years ago.

This means that you usually need to compile V8 on your own before
you can start to compile & install V8Js itself.

It is recommended to install the V8 version for V8Js off your system's
load path, so it doesn't interfere with the V8 library shipped with your
system's distribution.


Snapshots
---------

V8 has (optional) support for so-called snapshots which speed up startup
performance drastically.  Hence they are generally recommended for use.

There are two flavours of snapshots: internal & external.

Internal snapshots are built right into the V8 library (libv8.so file),
so there's no need to handle them specially.

Besides there are external snapshots (which are enabled unless configured
otherwise).  If V8 is compiled with these, then V8Js needs to provide two
"binary blobs" to V8, named `natives_blob.bin` and `snapshot_blob.bin`.
In that case copy those two files to the same directory `libv8.so` was
installed to.


Compile V8 5.6 and newer (using GN)
-----------------------------------

```
# Install required dependencies
sudo apt-get install build-essential curl git python libglib2.0-dev

cd /tmp

# Install depot_tools first (needed for source checkout)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# Download v8
fetch v8
cd v8

# (optional) If you'd like to build a certain version:
git checkout 6.4.388.18
gclient sync

# Setup GN
tools/dev/v8gen.py -vv x64.release -- is_component_build=true use_custom_libcxx=false

# Build
ninja -C out.gn/x64.release/

# Install to /opt/v8/
sudo mkdir -p /opt/v8/{lib,include}
sudo cp out.gn/x64.release/lib*.so out.gn/x64.release/*_blob.bin \
  out.gn/x64.release/icudtl.dat /opt/v8/lib/
sudo cp -R include/* /opt/v8/include/
```

On Debian Stretch you need to set RPATH on the installed libraries,
so the library loader finds the dependencies:

```
sudo apt-get install patchelf
for A in /opt/v8/lib/*.so; do sudo patchelf --set-rpath '$ORIGIN' $A; done
```

Compile php-v8js itself
-----------------------

```
cd /tmp
git clone https://github.com/phpv8/v8js.git
cd v8js
phpize
./configure --with-v8js=/opt/v8 LDFLAGS="-lstdc++"
make
make test
sudo make install
```

Then add `extension=v8js.so` to your php.ini file. If you have a separate configuration for CLI, add it there also.

V8Js' build system assumes that the `icudtl.dat` file is located next to the `libv8.so`
library file and compiles the path into the library itself.  If for whatever reason the
`icudtl.dat` file is stored at a different place during runtime, you need to set the
php.ini variable `v8js.icudtl_dat_path` to point to the file.  Otherwise locale-aware
features of V8 will not work as expected.
