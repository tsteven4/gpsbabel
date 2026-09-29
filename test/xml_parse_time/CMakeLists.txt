get_filename_component(mydir "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
set(mytarget tst_${mydir})
set(myname unit-${mydir})
add_executable(${mytarget})
target_sources(${mytarget} PRIVATE ${mytarget}.cc)
target_link_libraries(${mytarget} PRIVATE gpsbabel_unit_test_lib)

add_test(NAME ${myname} COMMAND ${mytarget})
