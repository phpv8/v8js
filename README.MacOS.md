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
./configure --with-v8js=/opt/homebrew CPPFLAGS="-DV8_COMPRESS_POINTERS"
make -j4
make test
make install
```
