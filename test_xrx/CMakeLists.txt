set(xrx_tests_HDR
    "debug_copy_move.h"
    "noop_archetypes.h"
    "observer_mock.h"
    )
set(xrx_tests_SRC
    "unknown.cpp"
    "tests_concepts_observer.cpp"
    "tests_operator_range.cpp"
    "tests_operator_from.cpp"
    "tests_operator_repeat.cpp"
    "tests_operator_concat.cpp"
    "tests_operator_flat_map.cpp"
    "tests_operator_window.cpp"
    "tests_utils_containers.cpp"
    "tests_operator_filter.cpp"
    "tests_operator_transform.cpp"
    "tests_operator_create.cpp"
    "tests_operator_interval.cpp"
    "tests_operator_observe_on.cpp"
    "tests_operator_publish.cpp"
    "tests_operator_subscribe_on.cpp"
    "tests_operator_take.cpp"
    "tests_operator_reduce.cpp"
    "tests_operator_iterate.cpp"
    "tests_operator_tap_do.cpp"
    "tests_operator_window_toggle.cpp"
    "tests_operator_start_with.cpp"
    "tests_subject.cpp"
    "tests_pipe.cpp"
    "tests_utils_maybe_shared_ptr.cpp"
    )
add_executable(tests_xrx ${xrx_tests_SRC} ${xrx_tests_HDR})
set_all_warnings(tests_xrx PRIVATE)
target_link_libraries(tests_xrx PRIVATE GTest_Integrated)
target_link_libraries(tests_xrx PRIVATE xrx)
set_property(TARGET tests_xrx PROPERTY CXX_STANDARD 23)

set_target_properties(tests_xrx PROPERTIES FOLDER tests)
new_test(xrx_ tests_xrx)
