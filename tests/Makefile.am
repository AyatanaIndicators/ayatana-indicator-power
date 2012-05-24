TESTS =
CLEANFILES =
check_PROGRAMS =

###
###
###

# stock UMB tests on user-visible strings
include $(srcdir)/Makefile.am.strings

check_LIBRARIES = libgtest.a
nodist_libgtest_a_SOURCES = \
  $(GTEST_SOURCE)/src/gtest-all.cc \
  $(GTEST_SOURCE)/src/gtest_main.cc

AM_CPPFLAGS = $(GTEST_CPPFLAGS) -I${top_srcdir}/src -Wall -Werror
AM_CXXFLAGS = $(GTEST_CXXFLAGS)

###
###
###

TEST_LIBS = \
	$(top_builddir)/src/libpower.la \
	$(INDICATOR_LIBS) \
	$(UPOWER_LIBS) \
        $(COVERAGE_LDFLAGS) \
	libgtest.a

TEST_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(UPOWER_CFLAGS) \
        $(INDICATOR_CFLAGS) \
        $(COVERAGE_CFLAGS)

TESTS += test-device
check_PROGRAMS += test-device
test_device_SOURCES = test-device.cc
test_device_LDADD = $(TEST_LIBS)
test_device_CPPFLAGS = $(TEST_CPPFLAGS)

TESTS += test-indicator
check_PROGRAMS += test-indicator
test_indicator_SOURCES = test-indicator.cc
test_indicator_LDADD = $(TEST_LIBS)
test_indicator_CPPFLAGS = $(TEST_CPPFLAGS)

###
###
###
