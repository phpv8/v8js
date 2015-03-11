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
    if test "$V8_API_VERSION" -lt 3024006 ; then
       AC_MSG_ERROR([libv8 must be version 3.24.6 or greater])
    fi
    AC_DEFINE_UNQUOTED([PHP_V8_API_VERSION], $V8_API_VERSION, [ ])
    AC_DEFINE_UNQUOTED([PHP_V8_VERSION], "$ac_cv_v8_version", [ ])
  else
    AC_MSG_ERROR([could not determine libv8 version])
  fi

  AC_CACHE_CHECK(for debuggersupport in v8, ac_cv_v8_debuggersupport, [
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <v8-debug.h>]],
                               [[v8::Debug::DisableAgent()]])],
    [ac_cv_v8_debuggersupport=yes],
    [ac_cv_v8_debuggersupport=no])
])

  if test "$ac_cv_v8_debuggersupport" = "yes"; then
    AC_DEFINE([ENABLE_DEBUGGER_SUPPORT], [1], [Enable debugger support in V8Js])
  fi

  AC_LANG_RESTORE
  LIBS=$old_LIBS
  LDFLAGS=$old_LDFLAGS
  CPPFLAGS=$old_CPPFLAGS

  if test "$V8_API_VERSION" -ge 3029036 ; then
    dnl building for v8 3.29.36 or later, which requires us to
    dnl initialize and provide a platform; hence we need to
    dnl link in libplatform to make our life easier.
    PHP_ADD_INCLUDE($V8_DIR)

    case $host_os in
      darwin* )
        static_link_extra="libv8_libplatform.a libv8_libbase.a"
        ;;
      * )
        static_link_extra="libv8_libplatform.a"
        ;;
    esac

    for static_link_extra_file in $static_link_extra; do
      AC_MSG_CHECKING([for $static_link_extra_file])

      for i in $PHP_V8JS $SEARCH_PATH ; do
        if test -r $i/lib/$static_link_extra_file; then
          static_link_dir=$i
          AC_MSG_RESULT(found in $i)
        fi
      done

      if test -z "$static_link_dir"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please provide $static_link_extra_file next to the libv8.so, see README.md for details])
      fi

      LDFLAGS="$LDFLAGS $static_link_dir/lib/$static_link_extra_file"
    done
  fi

  PHP_NEW_EXTENSION(v8js, [	\
    v8js_array_access.cc	\
    v8js.cc					\
    v8js_class.cc			\
    v8js_commonjs.cc		\
    v8js_convert.cc			\
    v8js_debug.cc			\
    v8js_exceptions.cc		\
    v8js_methods.cc			\
    v8js_object_export.cc	\
	v8js_timer.cc			\
	v8js_v8.cc				\
    v8js_v8object_class.cc	\
    v8js_variables.cc		\
  ], $ext_shared, , "-std="$ac_cv_v8_cstd)

  PHP_ADD_MAKEFILE_FRAGMENT
fi
