set(port_RxCpp_linesfrombytes_SRCs
    "main_RxCpp_linesfrombytes.cpp"
    )
add_executable(sample_RxCpp_linesfrombytes ${port_RxCpp_linesfrombytes_SRCs})
set_all_warnings(sample_RxCpp_linesfrombytes PRIVATE)
target_link_libraries(sample_RxCpp_linesfrombytes PRIVATE xrx_all)
set_property(TARGET sample_RxCpp_linesfrombytes PROPERTY CXX_STANDARD 23)
