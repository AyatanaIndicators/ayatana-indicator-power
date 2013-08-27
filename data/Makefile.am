BUILT_SOURCES=
CLEANFILES=
EXTRA_DIST=

#
# the indicator bus file
#

indicatorsdir = $(prefix)/share/unity/indicators
dist_indicators_DATA = com.canonical.indicator.power

#
# the upstart job file
#

upstart_jobsdir = $(datadir)/upstart/sessions
upstart_jobs_DATA = indicator-power.conf
upstart_jobs_in = $(upstart_jobs_DATA:.conf=.conf.in)
$(upstart_jobs_DATA): $(upstart_jobs_in)
	$(AM_V_GEN) $(SED) -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@
BUILT_SOURCES += $(upstart_jobs_DATA)
CLEANFILES += $(upstart_jobs_DATA)
EXTRA_DIST += $(upstart_jobs_in)

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
