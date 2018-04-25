#!/bin/sh
#
#  Copyright (c) 2017, Rice University.
#  See the file LICENSE for details.
#
#  Mark W. Krentel
#  March 2017
#
#  This script uses an hpctoolkit-externals install directory to build
#  a dyninst application.  It adds the include and lib directories for
#  binutils, boost, libelf, libdwarf, dyninst and zlib to the compile
#  line.
#
#  First, download and build externals.  Then, run this script with
#  the source file(s), any extra compiler options and the externals's
#  install directory (prefix) as the last option.
#
#  ./mk-dyninst.sh [options]... file.cpp ... ext-dir
#
#  If the build succeeds, the script writes the file 'env.sh' with
#  shell commands to set LD_LIBRARY_PATH.
#
#  Note: please don't commit gratuitous changes to CXX and CXXFLAGS.
#  If needed, edit the file for local systems (or put options on the
#  command line), just don't push the changes to github.
#

CXX=g++
CXXFLAGS='-g -O -std=c++11'

environ=env.sh

#------------------------------------------------------------

die() {
    if test "x$1" != x ; then
	echo "error: $@"
    fi
    cat - <<EOF
usage: ./mk-dyninst.sh [options] ... file.cpp ... ext-dir

  options = options added to the compile line
  file.cpp = C++ source file(s)
  ext-dir = path to externals install directory

EOF
    exit 1
}

#------------------------------------------------------------

# Scan the command line for:
#
#  1. externals directory at the end of the list
#  2. first C++ source file, to set the output file name
#  3. whether the command line includes -o itself

if test "x$1" = x ; then
    die
fi

marker='x-*-*-*-x'
EXTERNALS=
output=
has_dash_o=no

set -- "$@" "$marker"

while true
do
    arg="$1"
    shift

    # externals directory at the end of the command line,
    # delete it and the marker from the line
    if test "x$1" = "x$marker" ; then
	EXTERNALS="$arg"
	shift
	break
    fi

    # the first C++ source file sets the output file name
    if test "x$output" = x ; then
	case "$arg" in
	    *.cpp | *.cxx ) output=`expr "$arg" : '\(.*\)....'` ;;
	    *.cc ) output=`expr "$arg" : '\(.*\)...'` ;;
	    *.C ) output=`expr "$arg" : '\(.*\)..'` ;;
	esac
    fi

    # see if the command line includes -o itself
    if test "x$arg" = x-o ; then
	has_dash_o=yes
    fi

    set -- "$@" "$arg"
done

if test ! -d "${EXTERNALS}/symtabAPI/include" \
    || test ! -d "${EXTERNALS}/symtabAPI/lib"
then
    die "bad externals install directory: $EXTERNALS"
fi
EXTERNALS=`( cd "$EXTERNALS" && /bin/pwd )`

if test "x$output" = x ; then
    die "missing source file(s)"
fi

#------------------------------------------------------------

# Make command line and compile.

BINUTILS_INC="${EXTERNALS}/binutils/include"
BINUTILS_LIB="${EXTERNALS}/binutils/lib"

BOOST_INC="${EXTERNALS}/boost/include"
BOOST_LIB="${EXTERNALS}/boost/lib"

ELF_INC="${EXTERNALS}/libelf/include"
ELF_LIB="${EXTERNALS}/libelf/lib"

DWARF_INC="${EXTERNALS}/libdwarf/include"
DWARF_LIB="${EXTERNALS}/libdwarf/lib"

S=/home/johnmc/pkgs-src/dyninst/INSTALL-dyninst
DYNINST_INC="${S}/include"
DYNINST_LIB="${S}/lib"

ZLIB_INC="${EXTERNALS}/zlib/include"
ZLIB_LIB="${EXTERNALS}/zlib/lib"

if test "$has_dash_o" = no ; then
    set -- -o "$output" "$@"
fi

set --  \
    $CXXFLAGS  \
    "$@"  \
    -I${BINUTILS_INC}  \
    -I${BOOST_INC}  \
    -I${ELF_INC}  \
    -I${DWARF_INC}  \
    -I${DYNINST_INC}  \
    -I${ZLIB_INC}  \
    ${BINUTILS_LIB}/libbfd.a  \
    ${BINUTILS_LIB}/libiberty.a  \
    ${BINUTILS_LIB}/libopcodes.a  \
    -L${DYNINST_LIB}  \
    -lparseAPI  -linstructionAPI  -lsymtabAPI  \
    -ldynDwarf  -ldynElf  -lcommon  \
    -L${ELF_LIB}  -lelf  \
    -L${DWARF_LIB}  -ldwarf  \
    -L${ZLIB_LIB}  -lz  \
    -ldl

echo $CXX $@
echo

$CXX "$@"

test $? -eq 0 || die "compile failed"

#------------------------------------------------------------

# If the program built ok, then write env.sh with shell commands to
# set LD_LIBRARY_PATH.

rm -f "$environ"

cat >"$environ" <<EOF
#
#  Source me to set LD_LIBRARY_PATH
#

LD_LIBRARY_PATH="${BINUTILS_LIB}:\${LD_LIBRARY_PATH}"
LD_LIBRARY_PATH="${BOOST_LIB}:\${LD_LIBRARY_PATH}"
LD_LIBRARY_PATH="${ELF_LIB}:\${LD_LIBRARY_PATH}"
LD_LIBRARY_PATH="${DWARF_LIB}:\${LD_LIBRARY_PATH}"
LD_LIBRARY_PATH="${DYNINST_LIB}:\${LD_LIBRARY_PATH}"
LD_LIBRARY_PATH="${ZLIB_LIB}:\${LD_LIBRARY_PATH}"

export LD_LIBRARY_PATH
EOF
