set(all_compile_SRCS
    "tests_compile_xrx_all.cpp"
    )
add_executable(tests_all_compile ${all_compile_SRCS})
set_all_warnings(tests_all_compile PRIVATE)
target_link_libraries(tests_all_compile PRIVATE xrx_all)
set_property(TARGET tests_all_compile PROPERTY CXX_STANDARD 23)

set_target_properties(tests_all_compile PROPERTIES FOLDER tests)
new_test(all_compile    tests_all_compile)
