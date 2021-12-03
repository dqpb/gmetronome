AC_INIT([GMetronome],
	[0.1],
	[https://gitlab.gnome.org/dqpb/gmetronome/issues/new],
	[gmetronome],
	[https://gitlab.gnome.org/dqpb/gmetronome])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign -Wall])

#
# Programs
#
AC_PROG_SED
AC_PROG_CXX
PKG_PROG_PKG_CONFIG

#
# Compiler
#
AX_CXX_COMPILE_STDCXX([17],[noext],[mandatory])

#
# Libraries
#
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])
AM_PATH_GLIB_2_0

#
# Audio backend: Pulseaudio
#
AC_ARG_WITH([pulseaudio],
            [AS_HELP_STRING([--with-pulseaudio], [enable pulseaudio support])],
            [],
            [with_pulseaudio=check])

AS_IF([test "x$with_pulseaudio" != "xno"],
  [PKG_CHECK_MODULES([PULSEAUDIO], [libpulse-simple],
     have_pulseaudio=true
     AC_DEFINE([HAVE_PULSEAUDIO], [1], [Define if you have pulseaudio support]),
     have_pulseaudio=false
     AS_IF([test "x$with_pulseaudio" != "xcheck"], 
      	AC_MSG_FAILURE([--with-pulseaudio was given, but test for libpulse-simple failed])
     )
  )]
)

AM_CONDITIONAL([HAVE_PULSEAUDIO], [test "x$have_pulseaudio" = "xtrue"])

#
# Audio backend: Alsa
#
AC_ARG_WITH([alsa],
            [AS_HELP_STRING([--with-alsa], [enable alsa support])],
            [],
            [with_alsa=check])

AS_IF([test "x$with_alsa" != "xno"],
  [PKG_CHECK_MODULES([ALSA], [alsa],
     have_alsa=true
     AC_DEFINE([HAVE_ALSA], [1], [Define if you have alsa support]),
     have_alsa=false
     AS_IF([test "x$with_alsa" != "xcheck"], 
      	AC_MSG_FAILURE([--with-alsa was given, but test for libasound failed])
     )
  )]
)

AM_CONDITIONAL([HAVE_ALSA], [test "x$have_alsa" = "xtrue"])

#
# Default audio backend
#
AS_IF([test "x$have_pulseaudio" = "xtrue"],
  AC_SUBST([DEFAULT_AUDIO_BACKEND],[pulseaudio]),
  AS_IF([test "x$have_alsa" = "xtrue"],
    AC_SUBST([DEFAULT_AUDIO_BACKEND],[alsa]),
    AC_SUBST([DEFAULT_AUDIO_BACKEND],[none])
  )
)

GLIB_GSETTINGS

#
# Gettext i18n support
#
GETTEXT_PACKAGE=gmetronome
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [The domain to use with gettext])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.6])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.6])

PGRM_LOCALEDIR=[${datadir}/locale]
AC_SUBST(PGRM_LOCALEDIR)

AC_CONFIG_FILES([
 Makefile
 src/Makefile
 data/Makefile
 po/Makefile.in
])

AC_OUTPUT
