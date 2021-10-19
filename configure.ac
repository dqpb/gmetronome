AC_INIT([GMetronome],
	[0.1],
	[dqpb@mailbox.org],
	[gmetronome])

AM_INIT_AUTOMAKE([-Wall])

AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(17)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 src/resources/Makefile
 po/Makefile.in
])

# AM_PATH_GLIB_2_0 fills in the variable GLIB_COMPILE_RESOURCES.
AM_PATH_GLIB_2_0
GLIB_GSETTINGS

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])
PKG_CHECK_MODULES([PULSEAUDIO], [libpulse-simple])

#
# Internationalisation
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

#
# Create Makefiles
#
AC_OUTPUT
