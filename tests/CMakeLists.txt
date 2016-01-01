find_package(GMock)
include_directories(${GMOCK_INCLUDE_DIRS})

# dbustest
pkg_check_modules(DBUSTEST REQUIRED
                  dbustest-1>=14.04.0)
include_directories (SYSTEM ${DBUSTEST_INCLUDE_DIRS})

# add warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${C_WARNING_ARGS}")

# build the necessary schemas
set_directory_properties (PROPERTIES
                          ADDITIONAL_MAKE_CLEAN_FILES gschemas.compiled)
set_source_files_properties (gschemas.compiled GENERATED)

# make a local install of the sounds
set(XDG_DATA_HOME "${CMAKE_CURRENT_BINARY_DIR}/share")
add_definitions(-DXDG_DATA_HOME="${XDG_DATA_HOME}")
file(COPY 
     DIRECTORY "${CMAKE_SOURCE_DIR}/data/"
     DESTINATION "${XDG_DATA_HOME}/${CMAKE_PROJECT_NAME}/sounds"
     FILES_MATCHING PATTERN "*.ogg")

# GSettings:
# compile the ayatana-indicator-power schema into a gschemas.compiled file in this directory,
# and help the tests to find that file by setting -DSCHEMA_DIR
set (SCHEMA_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DSCHEMA_DIR="${SCHEMA_DIR}")
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas
                 OUTPUT_VARIABLE COMPILE_SCHEMA_EXECUTABLE
                 OUTPUT_STRIP_TRAILING_WHITESPACE)
add_custom_command (OUTPUT gschemas.compiled
                    DEPENDS ${CMAKE_BINARY_DIR}/data/org.ayatana.indicator.power.gschema.xml
                    COMMAND cp -f ${CMAKE_BINARY_DIR}/data/*gschema.xml ${SCHEMA_DIR}
                    COMMAND ${COMPILE_SCHEMA_EXECUTABLE} ${SCHEMA_DIR})

add_custom_target(
    gschemas-compiled ALL DEPENDS gschemas.compiled
)

# look for headers in our src dir, and also in the directories where we autogenerate files...
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_BINARY_DIR}/src)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

###
###

function(add_test_by_name name)
  set (TEST_NAME ${name})
  set (COVERAGE_TEST_TARGETS ${COVERAGE_TEST_TARGETS} ${TEST_NAME} PARENT_SCOPE)
  add_executable (${TEST_NAME} ${TEST_NAME}.cc)
  target_link_options(${TEST_NAME} PRIVATE -no-pie)
  add_test (${TEST_NAME} ${TEST_NAME})
  add_dependencies (${TEST_NAME} ayatanaindicatorpowerservice gschemas-compiled)
  target_link_libraries (${TEST_NAME} ayatanaindicatorpowerservice gtest ${DBUSTEST_LIBRARIES} ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBS} ${URLDISPATCHER_LIBRARIES} ${GMOCK_LIBRARIES})
endfunction()
add_test_by_name(test-notify)
add_test(NAME dear-reader-the-next-test-takes-80-seconds COMMAND true)
add_test_by_name(test-device)

set(COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  PARENT_SCOPE
)
