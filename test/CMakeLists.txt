if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  message(FATAL_ERROR "Please use CMakeLists.txt in the project root directory to generate a build system.")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Test)

add_library(gpsbabel_unit_test_lib STATIC test_setup.cc)
target_link_libraries(gpsbabel_unit_test_lib PUBLIC Qt6::Test gpsbabellib)
target_include_directories(gpsbabel_unit_test_lib PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

option(GPSBABEL_ENABLE_BENCHMARK "enable benchmarking." OFF)
if(GPSBABEL_ENABLE_BENCHMARK)
  target_compile_definitions(gpsbabel_unit_test_lib PUBLIC "ENABLE_BENCHMARK")
endif()

set(UNIT_TEST_DIRS
  parse
  xml_parse_time
)
# assume each directory creates a test with the name tst_ prepended to the directory name.
set(UNIT_TESTS ${UNIT_TEST_DIRS})
list(TRANSFORM UNIT_TESTS PREPEND tst_)

foreach(unit_subdir ${UNIT_TEST_DIRS})
  add_subdirectory(${unit_subdir})
endforeach()

add_custom_target(unit-tests
                  DEPENDS ${UNIT_TESTS}
                  VERBATIM)

find_program(CTEST_EXECUTABLE NAMES ctest)
if(CTEST_EXECUTABLE STREQUAL "CTEST_EXECUTABLE-NOTFOUND")
  message(WARNING "ctest not found.")
else()
  add_custom_target(check-unit
                    COMMAND ${CTEST_EXECUTABLE} --test-dir ${CMAKE_CURRENT_BINARY_DIR} --output-on-failure
                    DEPENDS unit-tests 
                    VERBATIM
                    USES_TERMINAL)
endif()
