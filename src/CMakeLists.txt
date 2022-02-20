# handwritten sources
set(SERVICE_MANUAL_SOURCES
    brightness.c
    datafiles.c
    device-provider-mock.c
    device-provider-upower.c
    device-provider.c
    device.c
    flashlight.c
    notifier.c
    testing.c
    service.c
    utils.c)

# generated sources
include(GdbusCodegen)
set(SERVICE_GENERATED_SOURCES)
add_gdbus_codegen_with_namespace(SERVICE_GENERATED_SOURCES dbus-repowerd
                                 com.lomiri
                                 Dbus
                                 ${CMAKE_SOURCE_DIR}/src/com.lomiri.Repowerd.xml)
add_gdbus_codegen_with_namespace(SERVICE_GENERATED_SOURCES dbus-battery
                                 org.ayatana.indicator.power
                                 Dbus
                                 ${CMAKE_SOURCE_DIR}/data/org.ayatana.indicator.power.Battery.xml)
add_gdbus_codegen_with_namespace(SERVICE_GENERATED_SOURCES dbus-testing
                                 org.ayatana.indicator.power
                                 Dbus
                                 ${CMAKE_SOURCE_DIR}/data/org.ayatana.indicator.power.Testing.xml)

if (ENABLE_LOMIRI_FEATURES)
    add_gdbus_codegen_with_namespace(SERVICE_GENERATED_SOURCES dbus-accounts-sound
                                     com.lomiri.touch
                                     Dbus
                                     /usr/share/accountsservice/interfaces/com.lomiri.touch.AccountsService.Sound.xml)
endif()

# add the bin dir to our include path so the code can find the generated header files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# the service library for tests to link against (basically, everything except main())
add_library(${SERVICE_LIB} STATIC ${SERVICE_MANUAL_SOURCES} ${SERVICE_GENERATED_SOURCES})
include_directories(${CMAKE_SOURCE_DIR})
link_directories(${SERVICE_DEPS_LIBRARY_DIRS})

# the executable: lib + main()
add_executable (${SERVICE_EXEC} main.c)
target_link_libraries (${SERVICE_EXEC} ${SERVICE_LIB} ${SERVICE_DEPS_LIBRARIES})
install (TARGETS ${SERVICE_EXEC} RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
