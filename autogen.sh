#! /bin/sh

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a git clone. You need a
# couple of extra tools to run this script successfully.
# 
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.

PROJECT="GMetronome"

AUTOCONF_REQUIRED_VERSION=2.54
AUTOMAKE_REQUIRED_VERSION=1.13.0
AUTOPOINT_REQUIRED_VERSION=0.19.6

check_version ()
{
    VERSION_A=$1
    VERSION_B=$2

    save_ifs="$IFS"
    IFS=.
    set dummy $VERSION_A 0 0 0
    MAJOR_A=$2
    MINOR_A=$3
    MICRO_A=$4
    set dummy $VERSION_B 0 0 0
    MAJOR_B=$2
    MINOR_B=$3
    MICRO_B=$4
    IFS="$save_ifs"

    if expr "$MAJOR_A" = "$MAJOR_B" > /dev/null; then
        if expr "$MINOR_A" \> "$MINOR_B" > /dev/null; then
           echo "yes (version $VERSION_A)"
        elif expr "$MINOR_A" = "$MINOR_B" > /dev/null; then
            if expr "$MICRO_A" \>= "$MICRO_B" > /dev/null; then
               echo "yes (version $VERSION_A)"
            else
                echo "Too old (version $VERSION_A)"
                DIE=1
            fi
        else
            echo "Too old (version $VERSION_A)"
            DIE=1
        fi
    elif expr "$MAJOR_A" \> "$MAJOR_B" > /dev/null; then
	echo "Major version might be too new ($VERSION_A)"
    else
	echo "Too old (version $VERSION_A)"
	DIE=1
    fi
}

printf "checking for autoconf >= $AUTOCONF_REQUIRED_VERSION ... "
if (autoreconf --version) < /dev/null > /dev/null 2>&1; then
    VER=`autoreconf --version | head -n 1 \
         | grep -iw autoconf | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    DIE=0
    check_version $VER $AUTOCONF_REQUIRED_VERSION
    if test "$DIE" -eq 1; then
	echo "Please upgrade the autoconf package and call me again."
	exit 1
    fi
else
    echo
    echo "  You must have autoconf installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at https://ftp.gnu.org/pub/gnu/autoconf/"
    echo
    exit 1;
fi

printf "checking for autopoint >= $AUTOPOINT_REQUIRED_VERSION ... "
if (autopoint --version) < /dev/null > /dev/null 2>&1; then
    VER=`autopoint --version \
         | grep autopoint | sed "s/.* \([0-9.]*\)/\1/"`
    DIE=0
    check_version $VER $AUTOPOINT_REQUIRED_VERSION
    if test "$DIE" -eq 1; then
	echo "Please upgrade the autopoint package and call me again."
	exit 1
    fi
else
    echo
    echo "  You must have autopoint installed to compile $PROJECT"
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the latest version from "
    echo "     https://ftp.gnu.org/pub/gnu/gettext/"
    echo
    exit 1
fi

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

autoreconf --verbose --install "$srcdir"

if test -z "$NOCONFIGURE"; then

    if test -n "$AUTOGEN_CONFIGURE_ARGS" || test -n "$*"; then
	echo
	echo "I am going to run $srcdir/configure with the following arguments:"
	echo
	echo "  $AUTOGEN_CONFIGURE_ARGS $@"
	echo
    else
	echo
	echo "I am going to run $srcdir/configure"
	echo
        echo "If you wish to pass additional arguments, please specify them "
        echo "on the $0 command line or set the AUTOGEN_CONFIGURE_ARGS "
        echo "environment variable."
        echo
    fi
    
    $srcdir/configure $AUTOGEN_CONFIGURE_ARGS "$@"
    RC=$?
    if test $RC -ne 0; then
      echo
      echo "Configure failed or did not finish!"
      echo
      exit $RC
    fi

    echo
    echo "Now type 'make' to compile $PROJECT."
    echo
fi
