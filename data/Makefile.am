
gsettings_in_file = com.canonical.indicator.power.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

dist_noinst_DATA = \
	com.canonical.indicator.power.gschema.xml \
	$(gsettings_in_file)

CLEANFILES = \
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = \
	$(gsettings_SCHEMAS:.xml=.valid)
