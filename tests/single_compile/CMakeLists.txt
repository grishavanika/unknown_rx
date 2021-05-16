set(all_compile_SRCS
    "tests_compile_xrx_all.cpp"
    )
add_executable(tests_all_compile ${all_compile_SRCS})
set_all_warnings(tests_all_compile PRIVATE)
target_link_libraries(tests_all_compile PRIVATE xrx_all)
