PHP_ARG_WITH(v8js, for V8 Javascript Engine,
[  --with-v8js           Include V8 JavaScript Engine])

if test "$PHP_V8JS" != "no"; then
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="include/v8.h"
  
  if test -r $PHP_V8JS/$SEARCH_FOR; then
    case $host_os in
      darwin* )
        # MacOS does not support --rpath
        ;;
      * )
        LDFLAGS="$LDFLAGS -Wl,--rpath=$PHP_V8JS/$PHP_LIBDIR"
        ;;
    esac
    V8_DIR=$PHP_V8JS
  else
    AC_MSG_CHECKING([for V8 files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        V8_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$V8_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the v8 distribution])
  fi

  PHP_ADD_INCLUDE($V8_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH(v8, $V8_DIR/$PHP_LIBDIR, V8JS_SHARED_LIBADD)
  PHP_SUBST(V8JS_SHARED_LIBADD)
  PHP_REQUIRE_CXX()


  AC_CACHE_CHECK(for C standard version, ac_cv_v8_cstd, [
    ac_cv_v8_cstd="c++11"
    old_CPPFLAGS=$CPPFLAGS
    AC_LANG_PUSH([C++])
    CPPFLAGS="-std="$ac_cv_v8_cstd
    AC_TRY_RUN([int main() { return 0; }],[],[ac_cv_v8_cstd="c++0x"],[])
    AC_LANG_POP([C++])
    CPPFLAGS=$old_CPPFLAGS
  ]);

  AC_CACHE_CHECK(how to allow c++11 narrowing, ac_cv_v8_narrowing, [
    ac_cv_v8_narrowing=""
    old_CXXFLAGS=$CXXFLAGS
    AC_LANG_PUSH([C++])
    CXXFLAGS="-std="$ac_cv_v8_cstd
    AC_TRY_RUN([int main() {
        struct { unsigned int x; } foo = {-1};
        (void) foo;
        return 0;
    }], [ ac_cv_v8_narrowing="" ], [
        CXXFLAGS="-Wno-c++11-narrowing -std="$ac_cv_v8_cstd
        AC_TRY_RUN([int main() {
            struct { unsigned int x; } foo = {-1};
            (void) foo;
            return 0;
        }], [ ac_cv_v8_narrowing="-Wno-c++11-narrowing" ], [
            CXXFLAGS="-Wno-narrowing -std="$ac_cv_v8_cstd
            AC_TRY_RUN([int main() {
                struct { unsigned int x; } foo = {-1};
                (void) foo;
                return 0;
            }], [ ac_cv_v8_narrowing="-Wno-narrowing" ], [
                AC_MSG_ERROR([cannot compile with narrowing])
            ], [])
        ], [])
    ], [])
    AC_LANG_POP([C++])
    CXXFLAGS=$old_CXXFLAGS
  ]);


  old_LIBS=$LIBS
  old_LDFLAGS=$LDFLAGS
  old_CPPFLAGS=$CPPFLAGS

  case $host_os in
    darwin* )
      # MacOS does not support --rpath
      LDFLAGS="-L$V8_DIR/$PHP_LIBDIR"
      ;;
    * )
      LDFLAGS="-Wl,--rpath=$V8_DIR/$PHP_LIBDIR -L$V8_DIR/$PHP_LIBDIR"
      ;;
  esac

  LIBS=-lv8
  CPPFLAGS="-I$V8_DIR/include -std=$ac_cv_v8_cstd"
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS

  AC_CACHE_CHECK(for V8 version, ac_cv_v8_version, [
AC_TRY_RUN([#include <v8.h>
#include <iostream>
#include <fstream>
using namespace std;

int main ()
{
	ofstream testfile ("conftestval");
	if (testfile.is_open()) {
		testfile << v8::V8::GetVersion();
		testfile << "\n";
		testfile.close();
		return 0;
	}
	return 1;
}], [ac_cv_v8_version=`cat ./conftestval|awk '{print $1}'`], [ac_cv_v8_version=NONE], [ac_cv_v8_version=NONE])
])

  if test "$ac_cv_v8_version" != "NONE"; then
    ac_IFS=$IFS
    IFS=.
    set $ac_cv_v8_version
    IFS=$ac_IFS
    V8_API_VERSION=`expr [$]1 \* 1000000 + [$]2 \* 1000 + [$]3`
    if test "$V8_API_VERSION" -lt 4006076 ; then
       AC_MSG_ERROR([libv8 must be version 4.6.76 or greater])
    fi
    AC_DEFINE_UNQUOTED([PHP_V8_API_VERSION], $V8_API_VERSION, [ ])
    AC_DEFINE_UNQUOTED([PHP_V8_VERSION], "$ac_cv_v8_version", [ ])
  else
    AC_MSG_ERROR([could not determine libv8 version])
  fi

  PHP_ADD_INCLUDE($V8_DIR)

  LDFLAGS_libplatform=""
  for static_link_extra_file in libv8_libplatform.a libv8_libbase.a; do
	AC_MSG_CHECKING([for $static_link_extra_file])
	static_link_dir=""

	if test -r $V8_DIR/$PHP_LIBDIR/$static_link_extra_file; then
	  static_link_dir=$V8_DIR/$PHP_LIBDIR
	  AC_MSG_RESULT(found in $V8_DIR/$PHP_LIBDIR)
	fi

	if test -z "$static_link_dir"; then
	  AC_MSG_RESULT([not found])
    else
      LDFLAGS_libplatform="$LDFLAGS_libplatform $static_link_dir/$static_link_extra_file"
	fi
  done

  # modify flags for (possibly) succeeding V8 startup check
  CPPFLAGS="$CPPFLAGS -I$V8_DIR"
  LIBS="$LIBS $LDFLAGS_libplatform"

  dnl building for v8 4.4.10 or later, which requires us to
  dnl provide startup data, if V8 wasn't compiled with snapshot=off.
  AC_MSG_CHECKING([whether V8 requires startup data])
  AC_TRY_RUN([
	  #include <v8.h>
	  #include <libplatform/libplatform.h>
	  #include <stdlib.h>
	  #include <string.h>

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
  virtual void* Allocate(size_t length) {
	  void* data = AllocateUninitialized(length);
	  return data == NULL ? data : memset(data, 0, length);
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};

	  int main ()
	  {
		  v8::Platform *v8_platform = v8::platform::CreateDefaultPlatform();
		  v8::V8::InitializePlatform(v8_platform);
		  v8::V8::Initialize();

		  static ArrayBufferAllocator array_buffer_allocator;
		  v8::Isolate::CreateParams create_params;
		  create_params.array_buffer_allocator = &array_buffer_allocator;

		  v8::Isolate::New(create_params);
		  return 0;
	  }
  ], [
	  AC_MSG_RESULT([no])
  ], [
	  AC_MSG_RESULT([yes])
	  AC_DEFINE([PHP_V8_USE_EXTERNAL_STARTUP_DATA], [1], [Whether V8 requires (and can be provided with custom versions of) external startup data])

	  SEARCH_PATH="$V8_DIR/$PHP_LIBDIR $V8_DIR/share/v8"

	  AC_MSG_CHECKING([for natives_blob.bin])
	  SEARCH_FOR="natives_blob.bin"

	  for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
		  AC_MSG_RESULT([found ($i/$SEARCH_FOR)])
		  AC_DEFINE_UNQUOTED([PHP_V8_NATIVES_BLOB_PATH], "$i/$SEARCH_FOR", [Full path to natives_blob.bin file])
		  native_blob_found=1
		fi
	  done

	  if test -z "$native_blob_found"; then
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([Please provide V8 native blob as needed])
	  fi

	  AC_MSG_CHECKING([for snapshot_blob.bin])
	  SEARCH_FOR="snapshot_blob.bin"

	  for i in $SEARCH_PATH ; do
		if test -r $i/$SEARCH_FOR; then
		  AC_MSG_RESULT([found ($i/$SEARCH_FOR)])
		  AC_DEFINE_UNQUOTED([PHP_V8_SNAPSHOT_BLOB_PATH], "$i/$SEARCH_FOR", [Full path to snapshot_blob.bin file])
		  snapshot_blob_found=1
		fi
	  done

	  if test -z "$snapshot_blob_found"; then
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([Please provide V8 snapshot blob as needed])
	  fi
  ])

  AC_LANG_RESTORE
  LIBS=$old_LIBS
  LDFLAGS="$old_LDFLAGS $LDFLAGS_libplatform"
  CPPFLAGS=$old_CPPFLAGS


  PHP_NEW_EXTENSION(v8js, [	\
    v8js_array_access.cc	\
    v8js.cc					\
    v8js_class.cc			\
    v8js_commonjs.cc		\
    v8js_convert.cc			\
    v8js_exceptions.cc		\
    v8js_generator_export.cc	\
    v8js_methods.cc			\
    v8js_object_export.cc	\
	v8js_timer.cc			\
	v8js_v8.cc				\
    v8js_v8object_class.cc	\
    v8js_variables.cc		\
  ], $ext_shared, , "$ac_cv_v8_narrowing -std="$ac_cv_v8_cstd)

  PHP_ADD_MAKEFILE_FRAGMENT
fi

dnl Local variables:
dnl tab-width: 4
dnl c-basic-offset: 4
dnl End:
dnl vim600: noet sw=4 ts=4 fdm=marker
dnl vim<600: noet sw=4 ts=4
