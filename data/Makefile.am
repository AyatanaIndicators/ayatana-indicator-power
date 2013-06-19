BUILT_SOURCES=
CLEANFILES=
EXTRA_DIST=

#
# the indicator bus file
#

indicatorsdir = $(prefix)/share/unity/indicators
dist_indicators_DATA = com.canonical.indicator.power

#
# the dbus service file
#

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-power.service
dbus_services_in = $(dbus_services_DATA:.service=.service.in)
$(dbus_services_DATA): $(dbus_services_in)
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
BUILT_SOURCES += $(dbus_services_DATA)
CLEANFILES += $(dbus_services_DATA)
EXTRA_DIST += $(dbus_services_in)

#
# the gettings
#

gsettings_in_file = com.canonical.indicator.power.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
CLEANFILES += $(gsettings_SCHEMAS)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

dist_noinst_DATA = \
	com.canonical.indicator.power.gschema.xml \
	$(gsettings_in_file)

CLEANFILES += \
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = \
	$(gsettings_SCHEMAS:.xml=.valid)
