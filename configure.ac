AC_INIT([indicator-power],
        [0.0.1],
        [http://bugs.launchpad.net/indicator-power],
        [indicator-power],
        [http://launchpad.net/indicator-power])

AC_PREREQ([2.64])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 -Wall foreign subdir-objects no-dist-gzip dist-xz])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT


###########################
# Dependencies
###########################

GLIB_REQUIRED_VERSION=2.28
GIO_UNIX_REQUIRED_VERSION=2.28
GTK_REQUIRED_VERSION=2.24
GTK3_REQUIRED_VERSION=3.0
INDICATOR_REQUIRED_VERSION=0.3.0
UPOWER_REQUIRED_VERSION=0.9.5

PKG_CHECK_MODULES([UPOWER],[upower-glib >= UPOWER_REQUIRED_VERSION])
PKG_CHECK_MODULES([INDICATOR],[
        gtk+-3.0 >= $GTK3_REQUIRED_VERSION
        indicator3 >= $INDICATOR_REQUIRED_VERSION
])

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE([localinstall],
              [AS_HELP_STRING([--enable-localinstall],
                              [install all of the files localy instead of system directories (for distcheck)])],
              [with_localinstall=$enableval],
              [with_localinstall=no])

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/3/"
	INDICATORICONSDIR="${datadir}/indicator-applet/icons/"
elif test "x$with_gtk" = x2; then
        INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
        INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
else
        INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3`
        INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3`
fi
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

IT_PROG_INTLTOOL([0.41.0])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_SUBST([GETTEXT_PACKAGE],[PACKAGE_TARNAME])
AC_DEFINE([GETTEXT_PACKAGE],[PACKAGE_TARNAME],[Define to the gettext package name.])

###########################
# Files
###########################

AC_CONFIG_FILES([
Makefile
po/Makefile.in
])
AC_OUTPUT

###########################
# Results
###########################

AC_MSG_NOTICE([

Power Indicator Configuration:

	Prefix:        $prefix
])
