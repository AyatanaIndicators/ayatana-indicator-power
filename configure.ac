AC_INIT([indicator-power],
        [13.10.0],
        [http://bugs.launchpad.net/indicator-power],
        [indicator-power],
        [http://launchpad.net/indicator-power])
AC_COPYRIGHT([Copyright 2011-2013 Canonical])

AC_PREREQ([2.64])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/device.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 -Wall foreign dist-xz check-news])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AR

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT


###########################
# Dependencies
###########################

GLIB_REQUIRED_VERSION=2.35.4
GIO_REQUIRED_VERSION=2.26
GIO_UNIX_REQUIRED_VERSION=2.26
GUDEV_REQUIRED_VERSION=204

PKG_CHECK_MODULES([SERVICE_DEPS],[glib-2.0 >= $GLIB_REQUIRED_VERSION
                                  gio-2.0 >= $GIO_REQUIRED_VERSION
                                  gio-unix-2.0 >= $GIO_UNIX_REQUIRED_VERSION
                                  gudev-1.0 >= $GUDEV_REQUIRED_VERSION])

###########################
# GSETTINGS
###########################

GLIB_GSETTINGS

###########################
# Google Test framework
###########################

AC_ARG_ENABLE([tests],
    [AS_HELP_STRING([--disable-tests], [Disable test scripts and tools (default=auto)])],
    [enable_tests=${enableval}],
    [enable_tests=auto])
if test "x$enable_tests" != "xno"; then
    m4_include([m4/gtest.m4])
    CHECK_GTEST
    if test "x$enable_tests" = "xauto"; then
        enable_tests=${have_gtest}
    elif test "x$enable_tests" = "xyes" && test "x$have_gtest" != "xyes"; then
        AC_MSG_ERROR([tests were requested but gtest is not installed.])
    fi
fi
AM_CONDITIONAL([BUILD_TESTS],[test "x$enable_tests" = "xyes"])

###########################
# gcov coverage reporting
###########################

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

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

IT_PROG_INTLTOOL([0.50.0])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_SUBST([GETTEXT_PACKAGE],[${PACKAGE_TARNAME}])
AC_DEFINE([GETTEXT_PACKAGE],[PACKAGE_TARNAME],[Define to the gettext package name.])
AC_DEFINE_PATH([GNOMELOCALEDIR],"${datadir}/locale",[locale directory])

###########################
# Files
###########################

AC_CONFIG_FILES([
Makefile
po/Makefile.in
data/Makefile
src/Makefile
tests/Makefile
])
AC_OUTPUT

###########################
# Results
###########################

AC_MSG_NOTICE([

Power Indicator Configuration:

	Prefix:        $prefix
	Unit Tests:    $enable_tests
	gcov:          $use_gcov

])
