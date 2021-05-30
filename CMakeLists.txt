cmake_minimum_required(VERSION 3.12.4 FATAL_ERROR)
# 3.12.4 is required because of C++20 support.
project(xrx)

# For proper work of CMAKE_EXPORT_COMPILE_COMMANDS.
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

include(tools/cmake/utils.cmake)
include(tools/cmake/clang_msvc_integration.cmake)
include(CTest)

# Enable folders for VS.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Leave only most Debug/Release configurations.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

detect_compilers()
set_cpp20_standard()

if (clang_on_msvc)
    make_clang_work_on_windows()
endif()

add_subdirectory(third_party)

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
    "include/debug/assert_mutex.h"
    "include/debug/tracking_allocator.h"
    "include/debug/new_allocs_stats.h"
    "include/debug/operator_new_override_for_stats.h"
    "include/debug/malloc_allocator.h"
    "include/debug/tracking_allocator.h"
    "include/utils_ref_tracking_observer.h"
    "include/simple_scheduler_event_loop.h"
    "include/operators/operator_window_toggle.h"
    "include/operators/operator_start_with.h"
    "include/xrx_prologue.h"
    "include/xrx_epilogue.h"
    )

add_library(xrx INTERFACE ${xrx_HEADERS})
target_include_directories(xrx INTERFACE "include/")

set(xrx_single_HEADERS
    "include_single/xrx/xrx_all.h"
    )

add_library(xrx_all INTERFACE ${xrx_single_HEADERS})
target_include_directories(xrx_all INTERFACE "include_single/")

add_subdirectory(tests)
add_subdirectory(examples)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "tests_xrx")
