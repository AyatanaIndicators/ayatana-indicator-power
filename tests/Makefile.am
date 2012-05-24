TESTS =
CLEANFILES =
check_PROGRAMS =

AM_CPPFLAGS = $(GTEST_CPPFLAGS) -I${top_srcdir}/src -Wall -Werror
AM_CXXFLAGS = $(GTEST_CXXFLAGS)

###
###
###

# stock UMB tests on user-visible strings
include $(srcdir)/Makefile.am.strings

###
###
###

TESTS += test-device
check_PROGRAMS += test-device
test_device_SOURCES = test-device.cc
test_device_LDADD = \
	$(top_builddir)/src/libpower.la \
	$(INDICATOR_LIBS) \
	$(UPOWER_LIBS) \
        $(COVERAGE_LDFLAGS) \
	libgtest.a

test_device_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(UPOWER_CFLAGS) \
        $(INDICATOR_CFLAGS) \
        $(COVERAGE_CFLAGS)

check_LIBRARIES = libgtest.a
nodist_libgtest_a_SOURCES = \
  $(GTEST_SOURCE)/src/gtest-all.cc \
  $(GTEST_SOURCE)/src/gtest_main.cc
