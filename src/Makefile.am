BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =

SHARED_CFLAGS = \
  -Wall -Wextra -Werror \
  $(SERVICE_DEPS_CFLAGS) \
  -DG_LOG_DOMAIN=\"Indicator-Power\"

###
###

upower_dbus_sources = \
  dbus-upower.c \
  dbus-upower.h

$(upower_dbus_sources): org.freedesktop.UPower.xml
	$(AM_V_GEN) gdbus-codegen \
	  --c-namespace Dbus \
	  --interface-prefix org.freedesktop \
	  --generate-c-code dbus-upower \
          $^

BUILT_SOURCES += $(upower_dbus_sources)
CLEANFILES += $(upower_dbus_sources)
EXTRA_DIST += org.freedesktop.UPower.xml

###
###
###

noinst_LIBRARIES = libindicatorpower-upower.a libindicatorpower-service.a

libindicatorpower_upower_a_SOURCES = \
  $(upower_dbus_sources) \
  device-provider-upower.c \
  device-provider-upower.h

libindicatorpower_upower_a_CFLAGS = \
	$(SHARED_CFLAGS) \
	-Wno-unused-parameter \
	$(COVERAGE_CFLAGS)

libindciatorpower_upower_a_LDFLAGS = $(COVERAGE_LDFLAGS)

libindicatorpower_service_a_SOURCES = \
  ib-brightness-control.c \
  ib-brightness-control.h \
  device-provider.c \
  device-provider.h \
  device.c \
  device.h \
  service.c \
  service.h

libindicatorpower_service_a_CFLAGS = \
	$(SHARED_CFLAGS) \
	-Wno-missing-field-initializers \
	$(COVERAGE_CFLAGS)

libindicatorpower_service_a_LDFLAGS = $(COVERAGE_LDFLAGS)

###
###
###

pkglibexec_PROGRAMS = indicator-power-service

indicator_power_service_SOURCES = main.c

indicator_power_service_CFLAGS = \
	$(SHARED_CFLAGS) \
	$(COVERAGE_CFLAGS)

indicator_power_service_LDADD = \
  libindicatorpower-upower.a \
  libindicatorpower-service.a \
  $(SERVICE_DEPS_LIBS)

indicator_power_service_LDFLAGS = \
  $(COVERAGE_LDFLAGS)
