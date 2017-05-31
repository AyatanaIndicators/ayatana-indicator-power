##
##  GSettings schema
##

include (UseGSettings)
set (SCHEMA_NAME "org.ayatana.indicator.power.gschema.xml")
set (SCHEMA_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_NAME}")
set (SCHEMA_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_NAME}.in")

# generate the .xml file using intltool
set (ENV{LC_ALL} "C")
execute_process (COMMAND intltool-merge -quiet --xml-style --utf8 --no-translations "${SCHEMA_FILE_IN}" "${SCHEMA_FILE}")

# let UseGSettings do the rest
add_schema (${SCHEMA_FILE})

##
##  Systemd Unit File
##

# where to install
# Uncomment when we drop Vivid
# pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
set (SYSTEMD_USER_DIR "/usr/lib/systemd/user")
message (STATUS "${SYSTEMD_USER_DIR} is the systemd user unit file install dir")

set (SYSTEMD_USER_NAME "${CMAKE_PROJECT_NAME}.service")
set (SYSTEMD_USER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SYSTEMD_USER_NAME}")
set (SYSTEMD_USER_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEMD_USER_NAME}.in")

# build it
set (pkglibexecdir "${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")
configure_file ("${SYSTEMD_USER_FILE_IN}" "${SYSTEMD_USER_FILE}")

# install it
install (FILES "${SYSTEMD_USER_FILE}"
         DESTINATION "${SYSTEMD_USER_DIR}")

##
##  Upstart systemd override Job File
##

set (UPSTART_SYSTEMD_OVERRIDE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/upstart/systemd-session/upstart")
message (STATUS "${UPSTART_SYSTEMD_OVERRIDE_DIR} is the Upstart override Job File for systemd dir")

install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.override"
         DESTINATION "${UPSTART_SYSTEMD_OVERRIDE_DIR}")

##
##  Upstart Job File
##

# where to install
set (UPSTART_JOB_DIR "${CMAKE_INSTALL_FULL_DATADIR}/upstart/sessions")
message (STATUS "${UPSTART_JOB_DIR} is the Upstart Job File install dir")

set (UPSTART_JOB_NAME "${CMAKE_PROJECT_NAME}.conf")
set (UPSTART_JOB_FILE "${CMAKE_CURRENT_BINARY_DIR}/${UPSTART_JOB_NAME}")
set (UPSTART_JOB_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${UPSTART_JOB_NAME}.in")

# build it
set (pkglibexecdir "${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")
configure_file ("${UPSTART_JOB_FILE_IN}" "${UPSTART_JOB_FILE}")

# install it
install (FILES "${UPSTART_JOB_FILE}"
         DESTINATION "${UPSTART_JOB_DIR}")

##
##  XDG Autostart File
##

# where to install
set (XDG_AUTOSTART_DIR "/etc/xdg/autostart")
message (STATUS "${XDG_AUTOSTART_DIR} is the DBus Service File install dir")

set (XDG_AUTOSTART_NAME "${CMAKE_PROJECT_NAME}.desktop")
set (XDG_AUTOSTART_FILE "${CMAKE_CURRENT_BINARY_DIR}/${XDG_AUTOSTART_NAME}")
set (XDG_AUTOSTART_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${XDG_AUTOSTART_NAME}.in")

# build it
set (pkglibexecdir "${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")
configure_file ("${XDG_AUTOSTART_FILE_IN}" "${XDG_AUTOSTART_FILE}")

# install it
install (FILES "${XDG_AUTOSTART_FILE}"
         DESTINATION "${XDG_AUTOSTART_DIR}")

##  Unity Indicator File
##

# where to install
set (AYATANA_INDICATOR_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/ayatana/indicators")
message (STATUS "${AYATANA_INDICATOR_DIR} is the Unity Indicator install dir")

set (AYATANA_INDICATOR_NAME "org.ayatana.indicator.power")
set (AYATANA_INDICATOR_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${AYATANA_INDICATOR_NAME}")

install (FILES "${AYATANA_INDICATOR_FILE}"
         DESTINATION "${AYATANA_INDICATOR_DIR}")
