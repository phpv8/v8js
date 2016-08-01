#!/bin/bash -xeu

echo "Attempting to install NTS PHP, NTS version '$PHP_NTS_VERSION'/ configure args '$PHP_CONFIGURE_ARGS'"
if [ "x$PHP_NTS_VERSION" = "x" -o "x$PHP_CONFIGURE_ARGS" = "x" ] ; then
	echo "Missing nts version or configuration arguments";
	exit 1;
fi
PHP_FOLDER="php-$PHP_NTS_VERSION"
PHP_INSTALL_DIR="$HOME/travis_cache/$PHP_FOLDER"
echo "Downloading $PHP_INSTALL_DIR\n"
if [ -x $PHP_INSTALL_DIR/bin/php ] ; then
	echo "PHP $PHP_NTS_VERSION already installed and in cache at $HOME/travis_cache/$PHP_FOLDER";
	exit 0
fi
# Remove cache if it somehow exists
rm -rf $HOME/travis_cache/
# Otherwise, put a minimal installation inside of the cache.
PHP_TAR_FILE="$PHP_FOLDER.tar.bz2"
curl --verbose https://secure.php.net/distributions/$PHP_TAR_FILE -o $PHP_TAR_FILE
tar xjf $PHP_TAR_FILE
pushd $PHP_FOLDER
./configure $PHP_CONFIGURE_ARGS --prefix=$HOME/travis_cache/$PHP_FOLDER
make -j5
make install
popd

# Optionally, can make cached data smaller
# If no test files have --PHPDBG--, then phpdbg can be removed
# If no test files have --POST-- (or GET, etc), then php-cgi can be removed

echo "PHP $PHP_NTS_VERSION already installed and in cache at $HOME/travis_cache/$PHP_FOLDER";
