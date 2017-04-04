#!/bin/sh
#
#  Copyright (c) 2017, Rice University.
#  See the file LICENSE for details.
#
#  Mark W. Krentel
#  March 2017
#
#  This script sets PERIOD and LD_PRELOAD as a simple version of
#  hpcrun.  This script is just a convenience.  You can always export
#  these variables manually.
#
#  Usage: ./run.sh [period] file.so ... program arg ...
#

period=
preload=

die() {
    echo "error: $@" 1>&2
    exit 1
}

if test "x$1" = x ; then
    cat <<EOF
usage: ./run.sh [period] file.so ... program arg ...

where period is time in micro-seconds
and file.so is a file to preload
EOF
    exit 0
fi

while test "x$1" != x
do
    arg="$1"
    case "$arg" in
	# period in micro-seconds
	[0-9]* )
	    period="$arg"
	    shift
	    ;;

	# preload .so file
	*.so )
	    case "$arg" in
		*/* ) ;;
		* ) arg="./${arg}" ;;
	    esac
	    preload="${preload} ${arg}"
	    shift
	    ;;

	# anything else is prog arg ...
	* )
	    break
	    ;;
    esac
done

if test "x$1" = x ; then
    die "missing application program"
fi

if test "x$preload" = x ; then
    die "missing .so file(s) to preload"
fi

if test "x$period" != x ; then
    export PERIOD="$period"
fi

export LD_PRELOAD="$preload"

exec "$@"
