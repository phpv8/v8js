#!/bin/bash -xeu
# Ensure that ~/travis_cache is a folder with 0 files in it.
# This avoids accidentally caching left over files in s3.
CACHE_DIR="$HOME/travis_cache"
if [ -d "$CACHE_DIR" ]; then
	if [ find "$CACHE_DIR" -mindepth 1 -print -quit | grep -q . ]; then
		echo "$CACHE_DIR was not empty, clearing"
		rm -rf $CACHE_DIR;
		mkdir "$CACHE_DIR"
	fi
else
	echo "$CACHE_DIR did not exist, creating an empty directory to cache"
	mkdir "$CACHE_DIR"
fi
