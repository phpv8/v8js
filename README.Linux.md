V8Js on GNU/Linux
=================

Installation of V8Js on GNU/Linux is pretty much straight forward.

The biggest hurdle actually is that you need a rather new v8 library.
However most distributions still ship the rusty version 3.14, publish
years ago, since Node.js requires such an old version.

This means that you usually need to compile v8 on your own before
you can start to compile & install v8js itself.

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
In that case copy those two files to `/usr/share/v8/...`.

Compile latest V8
-----------------


```
# Install `libicu-dev` if you haven't already:
sudo apt-get install libicu-dev

cd /tmp

# Install depot_tools first (needed for source checkout)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"

# Download v8
fetch v8
cd v8

# (optional) If you'd like to build a certain version:
git checkout 4.9.385.28
gclient sync

# use libicu of operating system
export GYP_DEFINES="use_system_icu=1"

# Build (with internal snapshots)
export GYPFLAGS="-Dv8_use_external_startup_data=0"
make native library=shared snapshot=on -j8

# Install to /usr
sudo mkdir -p /usr/lib /usr/include
sudo cp out/native/lib.target/lib*.so /usr/lib/
sudo cp -R include/* /usr/include
echo -e "create /usr/lib/libv8_libplatform.a\naddlib out/native/obj.target/tools/gyp/libv8_libplatform.a\nsave\nend" | sudo ar -M
```

Then add `extension=v8js.so` to your php.ini file. If you have a separate configuration for CLI, add it there also.

* If you don't want to overwrite the system copy of v8, replace `/usr` in
  the above commands with some other path like `/opt/v8` and then add
  `--with-v8js=/opt/v8` to the php-v8js `./configure` command below.

`libv8_libplatform.a` should not be copied directly since it's a thin
archive, i.e. it contains only pointers to the build objects, which
otherwise must not be deleted.  The simple mri-script converts the
thin archive to a normal archive.


Compile php-v8js itself
-----------------------

```
cd /tmp
git clone https://github.com/phpv8/v8js.git
cd v8js
phpize
./configure
make
make test
sudo make install
```
