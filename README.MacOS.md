V8Js on MacOS
=============

Installation of V8Js on MacOS is pretty much straight forward.
The ARM-based M1 chips also work just fine.

You can use [brew](https://brew.sh) to install `v8`.  This provides
both the library as well as necessary include files in `/opt/homebrew`.


Compile php-v8js itself
-----------------------

```
cd /tmp
git clone https://github.com/phpv8/v8js.git
cd v8js
phpize
./configure --with-v8js=/opt/homebrew CPPFLAGS="-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX"
make -j4
make test
make install
```

V8Js' build system assumes that the `icudtl.dat` file is located next to the `libv8.so`
library file and compiles the path into the library itself.  If for whatever reason the
`icudtl.dat` file is stored at a different place during runtime, you need to set the
php.ini variable `v8js.icudtl_dat_path` to point to the file.  Otherwise locale-aware
features of V8 will not work as expected.

To avoid having to configure `v8js.icudtl_dat_path` manually, you can symlink or copy the ICU data file into the default library location. For Homebrew users, run:

In case of a brew installed v8, run:

```
ln -sf /opt/homebrew/Cellar/v8/$(brew list --versions v8 | awk '{print $2}')/libexec/icudtl.dat /opt/homebrew/lib/icudtl.dat
```

This ensures V8Js will find `icudtl.dat` automatically and timezone/i18n support will work out of the box.
