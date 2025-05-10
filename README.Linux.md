V8Js on GNU/Linux
=================

Installation of V8Js on GNU/Linux is pretty much straight forward.

First you need to decide if you can go with a V8 library that is
shipped with your GNU/Linux distribution or whether you would want
to compile V8 on your own.

Many GNU/Linux distributions, for example Debian/Ubuntu, have
recent V8 library versions installable, that are ready to be used.
In order to go with these, just `sudo apt-get install libv8-dev`
(or similar, as it fits your distribution).

If you compile V8 on your own, it is recommended to install the V8
version for V8Js off your system's load path, so it doesn't interfere
with the V8 library shipped with your system's distribution.


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


Pointer Compression
-------------------

V8 versions 8.0 and higher enable pointer compression by default.
See [the design document](https://docs.google.com/document/d/10qh2-b4C5OtSg-xLwyZpEI5ZihVBPtn1xwKBbQC26yI/edit)
for details.

Hence if you use one of the recent version (which you really should),
then you

 a) either need to manually disable pointer compression during
    the build of the library by passing the
    `v8_enable_pointer_compression=false` flag to `v8gen.py`

 b) or compile php-v8js with pointer compression as well, by adding
    `CPPFLAGS="-DV8_COMPRESS_POINTERS"` to the `./configure` call.


Sandbox
-------

V8 has optional sandbox support.  You need to compile php-v8js with matching
configurations.  If your V8 library was called with sandbox support, you
need to pass the `-DV8_ENABLE_SANDBOX` flag to the configure call.

By default V8 currently enables this feature.
Many GNU/Linux distributions currently seem to have sandbox feature turned
off however.

If you configure it the wrong way round, you'll get runtime errors like this,
as soon as php-v8js tries to initialize V8:

```
Embedder-vs-V8 build configuration mismatch. On embedder side sandbox is DISABLED while on V8 side it's ENABLED.
```

In order to compile V8 with sandbox support off, pass `v8_enable_sandbox=false`
to v8gen.py invocation.


Compile V8 5.6 and newer (using GN)
-----------------------------------

```
# Install required dependencies
sudo apt-get install build-essential curl git python3 libglib2.0-dev

cd /tmp

# Install depot_tools first (needed for source checkout)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# Download v8
fetch v8
cd v8

# (optional) If you'd like to build a certain version:
git checkout 12.0.267.36
gclient sync -D

ARCH=$(uname -m)

# Setup GN
tools/dev/v8gen.py -vv ${ARCH}.release -- is_component_build=true use_custom_libcxx=false

# Build
ninja -C out.gn/${ARCH}.release/

# Install to /opt/v8/
sudo mkdir -p /opt/v8/{lib,include}
sudo cp out.gn/${ARCH}.release/lib*.so out.gn/${ARCH}.release/*_blob.bin \
  out.gn/${ARCH}.release/icudtl.dat /opt/v8/lib/
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
./configure --with-v8js=/opt/v8 LDFLAGS="-lstdc++" CPPFLAGS="-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX"
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
