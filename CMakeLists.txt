project(ayatana-indicator-power C CXX)
cmake_minimum_required(VERSION 2.8.9)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(PROJECT_VERSION "2.0.92")
set(PACKAGE ${CMAKE_PROJECT_NAME})

##
##  GNU standard installation directories
## 

include (GNUInstallDirs)
if (EXISTS "/etc/debian_version") # Workaround for libexecdir on debian
  set (CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}")
  set (CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif ()
set (CMAKE_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
set (CMAKE_INSTALL_FULL_PKGLIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")

##
## Gettext
##

set(GETTEXT_PACKAGE "ayatana-indicator-power")
add_definitions (-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
                 -DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

##
##  Check for prerequisites
##

find_package (PkgConfig REQUIRED)
include (CheckIncludeFile)
include (FindPkgConfig)

pkg_check_modules(SERVICE_DEPS REQUIRED
                  glib-2.0>=2.36
                  gio-2.0>=2.36
                  gio-unix-2.0>=2.36
                  libnotify>=0.7.6)

include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})

set(URL_DISPATCHER_1_REQUIRED_VERSION 1)
pkg_check_modules(
  URLDISPATCHER
  url-dispatcher-1>=${URL_DISPATCHER_1_REQUIRED_VERSION}
)
include_directories(${URLDISPATCHER_INCLUDE_DIRS})

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

##
##  Actual building
##

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(C_WARNING_ARGS "${C_WARNING_ARGS} -Weverything")
  set(C_WARNING_ARGS "${C_WARNING_ARGS} -Wno-c++98-compat -Wno-padded") # these are annoying
  set(C_WARNING_ARGS "${C_WARNING_ARGS} -Wno-documentation") # gtk-doc != doxygen
else()
  set(C_WARNING_ARGS "${C_WARNING_ARGS} -Wall -Wextra -Wpedantic -Wformat=2")
endif()
set(C_WARNING_ARGS "${C_WARNING_ARGS} -Wno-missing-field-initializers") # GActionEntry


include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

# actually build things
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)


# testing & coverage
enable_testing ()
add_subdirectory(tests)
find_package(CoverageReport)
ENABLE_COVERAGE_REPORT(
  TARGETS ${SERVICE_LIB} ${SERVICE_EXEC}
  TESTS ${COVERAGE_TEST_TARGETS}
  FILTER /usr/include ${CMAKE_BINARY_DIR}/*
)
