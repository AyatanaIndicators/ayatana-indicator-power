
#
# the indicator bus file
#

indicatorsdir = $(prefix)/share/unity/indicators
indicators_DATA = com.canonical.indicator.power

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

CLEANFILES = \
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = \
	$(gsettings_SCHEMAS:.xml=.valid)
