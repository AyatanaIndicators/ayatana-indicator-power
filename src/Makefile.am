ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

###################
# Indicator Stuff
###################

powerlibdir = $(INDICATORDIR)
powerlib_LTLIBRARIES = libpower.la

libpower_la_SOURCES = \
        indicator-power.c

libpower_la_CFLAGS = \
        $(UPOWER_CFLAGS) \
        $(INDICATOR_CFLAGS) \
	$(COVERAGE_CFLAGS) \
        -Wall -Werror \
        -DG_LOG_DOMAIN=\"Indicator-Power\"

libpower_la_LIBADD = \
        $(UPOWER_LIBS) \
        $(INDICATOR_LIBS)

libpower_la_LDFLAGS = \
	$(COVERAGE_LDFLAGS) \
        -module \
        -avoid-version

