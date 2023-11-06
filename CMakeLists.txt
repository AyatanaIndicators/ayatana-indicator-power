cmake_minimum_required(VERSION 3.13)
project(ayatana-indicator-power C CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(PROJECT_VERSION "23.10.1")
set(PACKAGE ${CMAKE_PROJECT_NAME})

# Options
option(ENABLE_TESTS "Enable all tests and checks" OFF)
option(ENABLE_COVERAGE "Enable coverage reports (includes enabling all tests and checks)" OFF)
option(ENABLE_WERROR "Treat all build warnings as errors" OFF)
option(ENABLE_LOMIRI_FEATURES "Build with Lomiri-specific libraries, schemas and media" OFF)
option(ENABLE_DEVICEINFO "Build with deviceinfo integration" OFF)
option(ENABLE_RDA "Build with RDA (remote desktop awareness) support" ON)

if(ENABLE_COVERAGE)
    set(ENABLE_TESTS ON)
    set(CMAKE_BUILD_TYPE "Coverage")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(ENABLE_WERROR)
    add_definitions("-Werror")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions("-Weverything")
else()
    add_definitions("-Wall")
endif()

add_definitions("-Wno-sign-compare") # Needed for GTest on Ubuntu

##
##  GNU standard installation directories
##

include (GNUInstallDirs)

##
## Gettext
##

set(GETTEXT_PACKAGE "ayatana-indicator-power")
add_definitions(
  -DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
  -DG_LOG_DOMAIN="${GETTEXT_PACKAGE}"
  -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
  -DLOW_BATTERY_SOUND="LowBattery.ogg"
)

##
##  Check for prerequisites
##

set(SERVICE_LIB "ayatanaindicatorpowerservice")
set(SERVICE_EXEC "ayatana-indicator-power-service")
add_definitions(-DSERVICE_EXEC="${SERVICE_EXEC}")

find_package (PkgConfig REQUIRED)
include (CheckIncludeFile)
include (FindPkgConfig)

set(
    SERVICE_DEPS REQUIRED
    glib-2.0>=2.36
    gio-2.0>=2.36
    gio-unix-2.0>=2.36
    libnotify>=0.7.6
    libayatana-common>=0.9.1
)

if (ENABLE_LOMIRI_FEATURES)
    list (
        APPEND
        SERVICE_DEPS
        lomiri-schemas
        lomiri-sounds
    )
    pkg_get_variable(LOMIRI_SOUNDSDIR lomiri-sounds soundsdir)
    add_definitions (
        -DLOMIRI_FEATURES_ENABLED
        -DLOMIRI_SOUNDSDIR="${LOMIRI_SOUNDSDIR}"
    )
endif ()

if (ENABLE_RDA)
    list (
        APPEND
        SERVICE_DEPS
        rda
    )
    add_definitions (
      -DRDA_ENABLED
    )
endif ()

pkg_check_modules (SERVICE_DEPS REQUIRED ${SERVICE_DEPS})
include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})

if (ENABLE_DEVICEINFO)
    pkg_check_modules (DEVICEINFO IMPORTED_TARGET "deviceinfo")
    include_directories (${DEVICEINFO_INCLUDE_DIRS})
endif ()

##
##  custom targets
##

set (ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
add_custom_target (dist
                   COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target (cppcheck COMMAND cppcheck --enable=all -q --error-exitcode=2 --inline-suppr
                   ${CMAKE_SOURCE_DIR}/src
                   ${CMAKE_SOURCE_DIR}/tests)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

# actually build things
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)

# testing & coverage
if (ENABLE_TESTS)
    enable_testing ()
    add_subdirectory(tests)
    if (ENABLE_COVERAGE)
        find_package(CoverageReport)
        ENABLE_COVERAGE_REPORT(
            TARGETS ${SERVICE_LIB} ${SERVICE_EXEC}
            TESTS ${COVERAGE_TEST_TARGETS}
            FILTER /usr/include ${CMAKE_BINARY_DIR}/*
        )
    endif ()
endif ()

# Display config info

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Unit tests: ${ENABLE_TESTS}")
message(STATUS "Build with -Werror: ${ENABLE_WERROR}")
message(STATUS "Build with Lomiri features: ${ENABLE_LOMIRI_FEATURES}")
