dnl
dnl  Copyright (c) 2017, Rice University.
dnl  See the file LICENSE for details.
dnl
dnl  Mark W. Krentel
dnl  May 2017
dnl
dnl  Build map-sum and libtrace.so with autotools and libtool.  This
dnl  more closely reflects the conditions in hpcrun.
dnl

AC_INIT([cplus-test], [1.0])
AC_COPYRIGHT([Copyright (c) 2017, Rice University.
See the file LICENSE for details.])

AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.15.0 foreign -Wall])
AM_MAINTAINER_MODE
AC_PREREQ(2.69)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_PROG_AR
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_CXX_C_O

# Don't build static libtrace.a, we don't use it.  This must come
# before AC_PROG_LIBTOOL.
enable_static=no
AC_PROG_LIBTOOL

# Install everything into one directory.  Simpler to run this way.
bindir="$prefix"
libdir="$prefix"

#------------------------------------------------------------
# CC and CXX path and version
#------------------------------------------------------------

CC_VERSION=unknown
version=`$CC --version 2>/dev/null`
if echo "$version" | grep -i copyright >/dev/null ; then
    CC_VERSION=`echo "$version" | head -1`
fi

CC_PATH=
base=`echo $CC | awk '{ print $1 }'`
case "$base" in
    /* ) CC_PATH="$base" ;;
    * )  AC_PATH_PROG([CC_PATH], [$base]) ;;
esac
if test "x$CC_PATH" = x ; then
    CC_PATH="$base"
fi

CXX_VERSION=unknown
version=`$CXX --version 2>/dev/null`
if echo "$version" | grep -i copyright >/dev/null ; then
    CXX_VERSION=`echo "$version" | head -1`
fi

CXX_PATH=
base=`echo $CXX | awk '{ print $1 }'`
case "$base" in
    /* ) CXX_PATH="$base" ;;
    * )  AC_PATH_PROG([CXX_PATH], [$base]) ;;
esac
if test "x$CXX_PATH" = x ; then
    CXX_PATH="$base"
fi

#------------------------------------------------------------
# CXXFLAGS for std C++11
#------------------------------------------------------------

AC_MSG_CHECKING([for CXXFLAGS for std C++11])

ORIG_CXXFLAGS="$CXXFLAGS"
cxx11_flag=
cxx11_avail=no

AC_LANG_PUSH([C++])

for opt in none -std=c++11 -std=c++0x -std=c++14 -std=c++1y
do
    case "$opt" in
	none ) cxx11_flag= ;;
	* ) cxx11_flag="$opt" ;;
    esac
    CXXFLAGS="$ORIG_CXXFLAGS $cxx11_flag"

    AC_COMPILE_IFELSE([
    AC_LANG_SOURCE([[
#include <list>
int mylen(std::list <int> lst)
{
    int ans = 0;
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        ans++;
    }
    return ans;
}
]])], [ans=yes], [ans=no])

    if test "$ans" = yes ; then
	cxx11_avail=yes
	break
    else
	cxx11_flag=no
    fi
done

AC_LANG_POP

AC_MSG_RESULT([$cxx11_flag])

if test "$cxx11_avail" != yes ; then
    AC_MSG_WARN([unable to find CXXFLAGS for std C++11])
    AC_MSG_WARN([add -std=... to CXXFLAGS and rerun configure])
    AC_MSG_ERROR()
fi

#------------------------------------------------------------

AC_OUTPUT

AC_MSG_NOTICE([--------------------------------------------------])

AC_MSG_NOTICE([host: $host])

AC_MSG_NOTICE()
AC_MSG_NOTICE([C compiler: $CC_VERSION])
AC_MSG_NOTICE([Path:  $CC_PATH])
AC_MSG_NOTICE([CC:     $CC])
AC_MSG_NOTICE([CFLAGS: $CFLAGS])

AC_MSG_NOTICE()
AC_MSG_NOTICE([C++ compiler: $CXX_VERSION])
AC_MSG_NOTICE([Path:  $CXX_PATH])
AC_MSG_NOTICE([CXX:      $CXX])
AC_MSG_NOTICE([CXXFLAGS: $CXXFLAGS])
AC_MSG_NOTICE()
