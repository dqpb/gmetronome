#!/bin/sh
# Run this to generate all the initial makefiles, etc.

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" && autoreconf --force --verbose --install
) || exit

test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
