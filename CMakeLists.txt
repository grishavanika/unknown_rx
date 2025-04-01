cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
# 3.12.4 is required because of C++23 support.
project(xrx)

include(cmake_utils.cmake)
include(cmake_clang_msvc.cmake)
include(CTest)

# Enable folders for VS.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Leave only most Debug/Release configurations.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

detect_compilers()

if (clang_on_msvc)
    make_clang_work_on_windows()
endif()

# third_party ----
add_library(GTest_Integrated INTERFACE)
find_package(GTest CONFIG REQUIRED)
target_link_libraries(GTest_Integrated INTERFACE
    GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

set(xrx_HEADERS
    "include/any_observer.h"
    "include/concepts_observable.h"
    "include/concepts_observer.h"
    "include/cpo_make_operator.h"
    "include/meta_utils.h"
    "include/observable_interface.h"
    "include/observables_factory.h"
    "include/operator_tags.h"
    "include/subject.h"
    "include/tag_invoke_with_extension.h"
    "include/utils_containers.h"
    "include/utils_observable.h"
    "include/utils_observers.h"
    "include/tag_invoke.hpp"
    "include/utils_wrappers.h"
    "include/operators/operator_take.h"
    "include/operators/operator_range.h"
    "include/operators/operator_observe_on.h"
    "include/operators/operator_filter.h"
    "include/operators/operator_publish.h"
    "include/operators/operator_create.h"
    "include/operators/operator_transform.h"
    "include/operators/operator_interval.h"
    "include/operators/operator_subscribe_on.h"
    "include/operators/operator_from.h"
    "include/operators/operator_repeat.h"
    "include/operators/operator_concat.h"
    "include/operators/operator_flat_map.h"
    "include/operators/operator_window.h"
    "include/operators/operator_reduce.h"
    "include/operators/operator_iterate.h"
    "include/operators/operator_tap_do.h"
    "include/operators/operator_as_async.h"
    "include/debug/assert_mutex.h"
    "include/debug/tracking_allocator.h"
    "include/debug/new_allocs_stats.h"
    "include/debug/operator_new_override_for_stats.h"
    "include/debug/malloc_allocator.h"
    "include/debug/tracking_allocator.h"
    "include/debug/copy_move_life_tracking.h"
    "include/utils_ref_tracking_observer.h"
    "include/utils_maybe_shared_ptr.h"
    "include/simple_scheduler_event_loop.h"
    "include/operators/operator_window_toggle.h"
    "include/operators/operator_start_with.h"
    "include/xrx_prologue.h"
    "include/xrx_epilogue.h"
    )

add_library(xrx INTERFACE ${xrx_HEADERS})
target_include_directories(xrx INTERFACE "include/")
set_property(TARGET xrx PROPERTY CXX_STANDARD 23)
if (clang_on_msvc)
    target_compile_options(xrx INTERFACE
        -Wno-ctad-maybe-unsupported
        -Wno-unknown-attributes
        -Wno-c++20-extensions
        -Wno-c++20-compat
        -Wno-exit-time-destructors
        -Wno-undefined-func-template
        -Wno-non-virtual-dtor
        -Wno-delete-non-abstract-non-virtual-dtor
        )
endif()

set(xrx_single_HEADERS
    "include_single/xrx/xrx_all.h"
    )

add_library(xrx_all INTERFACE ${xrx_single_HEADERS})
target_include_directories(xrx_all INTERFACE "include_single/")
set_property(TARGET xrx_all PROPERTY CXX_STANDARD 23)
if (clang_on_msvc)
    target_compile_options(xrx_all INTERFACE
        -Wno-ctad-maybe-unsupported
        -Wno-unknown-attributes
        -Wno-c++20-extensions
        -Wno-c++20-compat
        -Wno-exit-time-destructors
        -Wno-undefined-func-template
        -Wno-non-virtual-dtor
        -Wno-delete-non-abstract-non-virtual-dtor
        )
endif()

add_subdirectory(test_xrx)
add_subdirectory(test_single_compile)
add_subdirectory(sample_RxCpp_linesfrombytes)
if (WIN32)
    add_subdirectory(sample_UI_window_clicks)
endif ()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "tests_xrx")
