BUILT_SOURCES=
CLEANFILES=
EXTRA_DIST=

#
# the upstart xdg autostart override
#

xdg_autostartdir = $(datadir)/upstart/xdg/autostart
xdg_autostart_DATA = indicator-power.desktop
xdg_autostart_in = $(xdg_autostart_DATA:.desktop=.desktop.in)
$(xdg_autostart_DATA): $(xdg_autostart_in)
	$(AM_V_GEN) $(SED) -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@
BUILT_SOURCES += $(xdg_autostart_DATA)
CLEANFILES += $(xdg_autostart_DATA)
EXTRA_DIST += $(xdg_autostart_in)

