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

  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS

  CPPFLAGS="$CPPFLAGS -I$V8_DIR/include -std=$ac_cv_v8_cstd"
  LDFLAGS="$LDFLAGS -L$V8_DIR/$PHP_LIBDIR"

  AC_DEFUN([V8_CHECK_LINK], [
	AC_MSG_CHECKING([for libv8_libplatform])
    save_LIBS="$LIBS"
	LIBS="$LIBS $1 -lv8_libplatform -lv8"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
	  namespace v8 {
		namespace platform {
		  void* CreateDefaultPlatform(int thread_pool_size = 0);
		}
	  }
	], [ v8::platform::CreateDefaultPlatform(); ])], [
	  dnl libv8_libplatform.so found
	  AC_MSG_RESULT(found)
	  V8JS_SHARED_LIBADD="$1 -lv8_libplatform $V8JS_SHARED_LIBADD"
      $2
	], [ $3 ])
    LIBS="$save_LIBS"
  ])

  V8_CHECK_LINK([], [], [
    V8_CHECK_LINK([-lv8_libbase], [], [ AC_MSG_ERROR([could not find libv8_libplatform library]) ])
  ])


  dnl
  dnl Check for V8 version
  dnl (basic support for library linking assumed to be achieved above)
  dnl
  LIBS="$LIBS $V8JS_SHARED_LIBADD"
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


  dnl
  dnl  Scan for blob.bin files (that might be needed)
  dnl
  AC_DEFUN([V8_SEARCH_BLOB], [
	AC_MSG_CHECKING([for $1])
	blob_found=0

	for i in "$V8_DIR/$PHP_LIBDIR" "$V8_DIR/share/v8"; do
	  if test -r "$i/$1"; then
		AC_MSG_RESULT([found ($i/$1)])
		AC_DEFINE_UNQUOTED([$2], "$i/$1", [Full path to $1 file])
		blob_found=1
	  fi
	done

	if test "$blob_found" -ne 1; then
	  AC_MSG_RESULT([not found])
	fi
  ])
  V8_SEARCH_BLOB([natives_blob.bin], [PHP_V8_NATIVES_BLOB_PATH])
  V8_SEARCH_BLOB([snapshot_blob.bin], [PHP_V8_SNAPSHOT_BLOB_PATH])


  AC_LANG_RESTORE
  LIBS=$old_LIBS
  LDFLAGS="$old_LDFLAGS"
  CPPFLAGS=$old_CPPFLAGS

  PHP_ADD_INCLUDE($V8_DIR)
  PHP_NEW_EXTENSION(v8js, [	\
    v8js_array_access.cc	\
    v8js_class.cc			\
    v8js_commonjs.cc		\
    v8js_convert.cc			\
    v8js_exceptions.cc		\
    v8js_generator_export.cc	\
    v8js_main.cc			\
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
