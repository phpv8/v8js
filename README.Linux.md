V8Js on GNU/Linux
=================

Installation of V8Js on GNU/Linux is pretty much straight forward.

The biggest hurdle actually is that you need a rather new v8 library.
However most distributions still ship the rusty version 3.14, publish
years ago, since Node.js requires such an old version.

This means that you usually need to compile v8 on your own before
you can start to compile & install v8js itself.

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

# Build (disable snapshots for V8 > 4.4.9.1)
make native library=shared snapshot=off -j8

# Install to /usr
sudo mkdir -p /usr/lib /usr/include
sudo cp out/native/lib.target/lib*.so /usr/lib/
sudo cp -R include/* /usr/include
echo -e "create /usr/lib/libv8_libplatform.a\naddlib out/native/obj.target/tools/gyp/libv8_libplatform.a\nsave\nend" | sudo ar -M
```

Then add `extension=v8js.so` to your php.ini file. If you have a separate configuration for CLI, add it there also.

* If the V8 library is newer than 4.4.9.1 you need to pass `snapshot=off` to
  `make`, otherwise the V8 library will not be usable
  (see V8 [Issue 4192](https://code.google.com/p/v8/issues/detail?id=4192))
* If you don't want to overwrite the system copy of v8, replace `/usr` in
  the above commands with some other path like `/opt/v8` and then add
  `--with-v8js=/opt/v8` to the php-v8js `./configure` command below.
* If you do that with a v8 library of 4.2 branch or newer, then you need
  to fix the RUNPATH header in the v8js.so library so the libicui18n.so
  is found. By default it is set to `$ORIGIN/lib.target/`, however the files
  lie side by side. Use `chrpath -r '$ORIGIN' libv8.so` to fix.

`libv8_libplatform.a` should not be copied directly since it's a thin
archive, i.e. it contains only pointers to the build objects, which
otherwise must not be deleted.  The simple mri-script converts the
thin archive to a normal archive.


Compile php-v8js itself
-----------------------

```
cd /tmp
git clone https://github.com/preillyme/v8js.git
cd v8js
phpize
./configure
make
make test
sudo make install
```
