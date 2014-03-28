TESTS =
CLEANFILES = 
BUILT_SOURCES =
check_PROGRAMS =

###
###  tests: stock tests on user-visible strings
###

include $(srcdir)/Makefile.am.strings

###
###  gtest library
###

check_LIBRARIES = libgtest.a
nodist_libgtest_a_SOURCES = \
  $(GTEST_SOURCE)/src/gtest-all.cc \
  $(GTEST_SOURCE)/src/gtest_main.cc

AM_CPPFLAGS = $(GTEST_CPPFLAGS) -I${top_srcdir}/src -Wall -Werror -std=c++11
AM_CXXFLAGS = $(GTEST_CXXFLAGS)

###
###  tests: indicator-power-device
###

dear-reader-please-note-the-next-test-takes-90-seconds:
	@echo "#!/bin/bash" > $@
	@echo "exit 0" >> $@
	@chmod +x $@
TESTS += dear-reader-please-note-the-next-test-takes-90-seconds
CLEANFILES += dear-reader-please-note-the-next-test-takes-90-seconds

TEST_LIBS = $(COVERAGE_LDFLAGS) libgtest.a -lpthread
TEST_CPPFLAGS = $(SERVICE_DEPS_CFLAGS) $(AM_CPPFLAGS)

TESTS += test-device
check_PROGRAMS += test-device
test_device_SOURCES = test-device.cc
test_device_CPPFLAGS = $(TEST_CPPFLAGS) -Wall -Werror -Wextra
test_device_LDADD = \
  $(top_builddir)/src/libindicatorpower-service.a \
  $(SERVICE_DEPS_LIBS) \
  $(TEST_LIBS)


# (FIXME: incomplete)
#
###
###  tests: indicator-power-service
###
#
# build a local copy of the GSettings schemas
#BUILT_SOURCES += gschemas.compiled
#CLEANFILES += gschemas.compiled
#gschemas.compiled: Makefile
#	@glib-compile-schemas --targetdir=$(abs_builddir) $(top_builddir)/data
#
#TESTS += test-service
#check_PROGRAMS += test-service
#test_service_SOURCES = test-service.cc
#test_service_LDADD = $(TEST_LIBS)
#test_service_CPPFLAGS = $(TEST_CPPFLAGS) -DSCHEMA_DIR="\"$(top_builddir)/tests/\""
#
#TESTS += test-dbus-listener
#check_PROGRAMS += test-dbus-listener
#test_dbus_listener_SOURCES = test-dbus-listener.cc
#test_dbus_listener_LDADD = $(TEST_LIBS)
#test_dbus_listener_CPPFLAGS = $(TEST_CPPFLAGS)
#
