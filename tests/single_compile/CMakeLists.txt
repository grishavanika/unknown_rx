set(single_compile_SRCS
    "../noop_archetypes.h"
    "tests_compile_range.cpp"
    "tests_compile_take.cpp"
    "tests_compile_transform.cpp"
    "tests_compile_subscribe_on.cpp"
    "tests_compile_publish.cpp"
    "tests_compile_interval.cpp"
    "tests_compile_observe_on.cpp"
    "tests_compile_filter.cpp"
    "tests_compile_create.cpp"
    )
add_executable(tests_single_compile ${single_compile_SRCS})
set_all_warnings(tests_single_compile PRIVATE)
target_include_directories(tests_single_compile PRIVATE "../..")
target_link_libraries(tests_single_compile PRIVATE GTest_Integrated)
target_link_libraries(tests_single_compile INTERFACE xrx)
# XXX: temporary. Why xrx includes do not work ?
target_include_directories(tests_single_compile PRIVATE "../../include")

set(all_compile_SRCS
    "tests_compile_xrx_all.cpp"
    )
add_executable(tests_all_compile ${all_compile_SRCS})
set_all_warnings(tests_all_compile PRIVATE)
target_include_directories(tests_all_compile PRIVATE "../..")
target_link_libraries(tests_all_compile PRIVATE GTest_Integrated)
target_link_libraries(tests_all_compile INTERFACE xrx_all)
# XXX: temporary. Why xrx includes do not work ?
target_include_directories(tests_all_compile PRIVATE "../../include_single")
