# build libgtest
add_library (gtest STATIC 
             ${GTEST_SOURCE_DIR}/gtest-all.cc 
             ${GTEST_SOURCE_DIR}/gtest_main.cc)
set_target_properties (gtest PROPERTIES INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${GTEST_INCLUDE_DIR})
set_target_properties (gtest PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS} -w)

# dbustest
pkg_check_modules(DBUSTEST REQUIRED
                  dbustest-1>=14.04.0)
include_directories (SYSTEM ${DBUSTEST_INCLUDE_DIRS})

# add warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${C_WARNING_ARGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-global-constructors") # Google Test

# build the necessary schemas
set_directory_properties (PROPERTIES
                          ADDITIONAL_MAKE_CLEAN_FILES gschemas.compiled)
set_source_files_properties (gschemas.compiled GENERATED)

# GSettings:
# compile the indicator-power schema into a gschemas.compiled file in this directory,
# and help the tests to find that file by setting -DSCHEMA_DIR
set (SCHEMA_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DSCHEMA_DIR="${SCHEMA_DIR}")
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas
                 OUTPUT_VARIABLE COMPILE_SCHEMA_EXECUTABLE
                 OUTPUT_STRIP_TRAILING_WHITESPACE)
add_custom_command (OUTPUT gschemas.compiled
                    DEPENDS ${CMAKE_BINARY_DIR}/data/com.canonical.indicator.power.gschema.xml
                    COMMAND cp -f ${CMAKE_BINARY_DIR}/data/*gschema.xml ${SCHEMA_DIR}
                    COMMAND ${COMPILE_SCHEMA_EXECUTABLE} ${SCHEMA_DIR})

# look for headers in our src dir, and also in the directories where we autogenerate files...
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_BINARY_DIR}/src)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

###
###

function(add_test_by_name name)
  set (TEST_NAME ${name})
  add_executable (${TEST_NAME} ${TEST_NAME}.cc gschemas.compiled)
  add_test (${TEST_NAME} ${TEST_NAME})
  add_dependencies (${TEST_NAME} libindicatorpowerservice)
  target_link_libraries (${TEST_NAME} indicatorpowerservice gtest ${DBUSTEST_LIBRARIES} ${SERVICE_DEPS_LIBRARIES} ${GTEST_LIBS})
endfunction()
add_test_by_name(test-notify)
add_test(NAME dear-reader-the-next-test-takes-80-seconds COMMAND true)
add_test_by_name(test-device)
