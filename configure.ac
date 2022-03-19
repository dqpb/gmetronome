AC_COPYRIGHT([Copyright (c) 2020-2022 The GMetronome Team. All rights reserved.])

AC_INIT([GMetronome],
	[0.1.0],
	[https://gitlab.gnome.org/dqpb/gmetronome/issues],
	[gmetronome],
	[https://gitlab.gnome.org/dqpb/gmetronome])

package_id="org.gnome.dqpb.GMetronome"

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign -Wall])

#
# Endianess
#
AC_C_BIGENDIAN

#
# Programs
#
AC_PROG_SED
AC_PROG_INSTALL
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG
AC_PROG_CXX

#
# Compiler
#
AX_CXX_COMPILE_STDCXX([17],[noext],[mandatory])

#
# Check C++ std lib features
#
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[#include <charconv>]],
  [[const char* str = "3.14";
    double value;
    std::from_chars(str, str + 4, value);]])],
  [AC_DEFINE([HAVE_CPP_LIB_TO_CHARS], [1],
    [Define if standard lib supports std::to_chars])]
)
AC_LANG_POP([C++])

#
# Header files
#
AC_HEADER_ASSERT

#
# Libraries
#
PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])
AM_PATH_GLIB_2_0

GLIB_GSETTINGS

#
# Host OS specific configuration
#
default_with_alsa=check
default_with_oss=check
default_with_pulseaudio=check
preferred_audio_backend=pulseaudio

case "$host_os" in
     linux*)
	default_with_alsa=check
	default_with_oss=no
	default_with_pulseaudio=check
	preferred_audio_backend=pulseaudio
	platform_linux=yes
	;;
     *)
	platform_linux=no
	;;
esac

case "$host_os" in
     freebsd*)
	default_with_alsa=no
	default_with_oss=check
	default_with_pulseaudio=check
	preferred_audio_backend=oss
	platform_freebsd=yes
	;;
     *)
	platform_freebsd=no
	;;
esac

#
# Audio backend: Alsa
#
AC_ARG_WITH([alsa],
	    [AS_HELP_STRING([--with-alsa], [enable alsa support])],
	    [],
	    [with_alsa=$default_with_alsa])

AS_IF([test "x$with_alsa" != "xno"],
  [PKG_CHECK_MODULES([ALSA], [alsa],
     have_alsa=yes
     AC_DEFINE([HAVE_ALSA], [1], [Define if you have alsa support]),
     have_alsa=no
     AS_IF([test "x$with_alsa" != "xcheck"],
	AC_MSG_FAILURE([--with-alsa was given but test for libasound failed])
     )
  )],
  [have_alsa=no]
)

AM_CONDITIONAL([HAVE_ALSA], [test "x$have_alsa" = "xyes"])

#
# Audio backend: OSS
#
AC_ARG_WITH([oss],
	    [AS_HELP_STRING([--with-oss], [enable oss support])],
	    [],
	    [with_oss=$default_with_oss])

AS_IF([test "x$with_oss" != "xno"],
  [AC_CHECK_HEADER([sys/soundcard.h],
     have_oss=yes
     AC_DEFINE([HAVE_OSS], [1], [Define if you have oss support]),
     have_oss=no
     AS_IF([test "x$with_oss" != "xcheck"],
	AC_MSG_FAILURE([--with-oss was given but test for sys/soundcard.h failed])
     )
  )],
  [have_oss=no]
)

AM_CONDITIONAL([HAVE_OSS], [test "x$have_oss" = "xyes"])

#
# Audio backend: Pulseaudio
#
AC_ARG_WITH([pulseaudio],
	    [AS_HELP_STRING([--with-pulseaudio], [enable pulseaudio support])],
	    [],
	    [with_pulseaudio=$default_with_pulseaudio])

AS_IF([test "x$with_pulseaudio" != "xno"],
  [PKG_CHECK_MODULES([PULSEAUDIO], [libpulse-simple],
     have_pulseaudio=yes
     AC_DEFINE([HAVE_PULSEAUDIO], [1], [Define if you have pulseaudio support]),
     have_pulseaudio=no
     AS_IF([test "x$with_pulseaudio" != "xcheck"],
	AC_MSG_FAILURE([--with-pulseaudio was given but test for libpulse-simple failed])
     )
  )],
  [have_pulseaudio=no]
)

AM_CONDITIONAL([HAVE_PULSEAUDIO], [test "x$have_pulseaudio" = "xyes"])

#
# Default audio backend
#
AS_IF([AS_VAR_IF([have_$preferred_audio_backend], [yes], [true], [false])],
	[AC_SUBST([DEFAULT_AUDIO_BACKEND],[$preferred_audio_backend])],
      [test "x$have_pulseaudio" = "xyes"],
	[AC_SUBST([DEFAULT_AUDIO_BACKEND],[pulseaudio])],
      [test "x$have_alsa" = "xyes"],
	[AC_SUBST([DEFAULT_AUDIO_BACKEND],[alsa])],
      [test "x$have_oss" = "xyes"],
	[AC_SUBST([DEFAULT_AUDIO_BACKEND],[oss])],
      [AC_SUBST([DEFAULT_AUDIO_BACKEND],[none])]
)

#
# Output application id
#
AC_SUBST([PACKAGE_ID], $package_id)
AC_DEFINE_UNQUOTED([PACKAGE_ID], "$package_id",
		   [Application ID])

AC_SUBST([PACKAGE_ID_PATH], "/${package_id//.//}")
AC_DEFINE_UNQUOTED([PACKAGE_ID_PATH], "/${package_id//.//}",
   [Application ID (PACKAGE_ID) with dots replaced by slashes including a leading slash])

#
# Create gschema.xml from template
#
AC_SUBST([GSCHEMAXML_DEFAULT_AUDIO_BACKEND],[\'$DEFAULT_AUDIO_BACKEND\'])

AS_IF([test "x$have_alsa" = "xyes"],
      [AC_SUBST([GSCHEMAXML_ALSA_BEGIN],[""]) AC_SUBST([GSCHEMAXML_ALSA_END],[""])],
      [AC_SUBST([GSCHEMAXML_ALSA_BEGIN],["<!--"]) AC_SUBST([GSCHEMAXML_ALSA_END],["-->"])]
)
AS_IF([test "x$have_oss" = "xyes"],
      [AC_SUBST([GSCHEMAXML_OSS_BEGIN],[""]) AC_SUBST([GSCHEMAXML_OSS_END],[""])],
      [AC_SUBST([GSCHEMAXML_OSS_BEGIN],["<!--"]) AC_SUBST([GSCHEMAXML_OSS_END],["-->"])]
)
AS_IF([test "x$have_pulseaudio" = "xyes"],
      [AC_SUBST([GSCHEMAXML_PULSEAUDIO_BEGIN],[""]) AC_SUBST([GSCHEMAXML_PULSEAUDIO_END],[""])],
      [AC_SUBST([GSCHEMAXML_PULSEAUDIO_BEGIN],["<!--"]) AC_SUBST([GSCHEMAXML_PULSEAUDIO_END],["-->"])]
)

AC_CONFIG_FILES([
  data/org.gnome.dqpb.GMetronome.gschema.xml
  data/org.gnome.dqpb.GMetronome.metainfo.xml.in
  data/org.gnome.dqpb.GMetronome.desktop.in
  src/resources/org.gnome.dqpb.GMetronome.gresource.xml
])

#
# Gettext i18n support
#
GETTEXT_PACKAGE=gmetronome
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [The domain to use with gettext])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.6])

PGRM_LOCALEDIR=[${datadir}/locale]
AC_SUBST(PGRM_LOCALEDIR)

AC_CONFIG_FILES([
 Makefile
 src/Makefile
 data/Makefile
 po/Makefile.in
])

AC_OUTPUT

summary_msg="
Building AC_PACKAGE_NAME with the following configuration:

General:
  prefix             : $prefix
  datarootdir        : $datarootdir

Audio backends:
  Alsa               : $have_alsa
  OSS                : $have_oss
  Pulseaudio         : $have_pulseaudio
  Default            : $DEFAULT_AUDIO_BACKEND
"

AC_MSG_RESULT($summary_msg)
