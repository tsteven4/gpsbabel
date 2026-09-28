if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  message(FATAL_ERROR "Please use CMakeLists.txt in the project root directory to generate a build system.")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(gpsbabel_unit_test)
target_sources(gpsbabel_unit_test PRIVATE test_setup.cc test_xml_parse_time.cc)
target_link_libraries(gpsbabel_unit_test PRIVATE Qt6::Test gpsbabellib)
target_include_directories(gpsbabel_unit_test PRIVATE ${CMAKE_SOURCE_DIR})

enable_testing(true)
add_test(NAME gpsbabel_unit_test COMMAND gpsbabel_unit_test)

