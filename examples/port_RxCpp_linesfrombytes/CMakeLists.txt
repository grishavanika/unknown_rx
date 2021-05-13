set(port_RxCpp_linesfrombytes_SRCs
    "main_RxCpp_linesfrombytes.cpp"
    )
add_executable(port_RxCpp_linesfrombytes ${port_RxCpp_linesfrombytes_SRCs})
set_all_warnings(port_RxCpp_linesfrombytes PRIVATE)
target_link_libraries(port_RxCpp_linesfrombytes PRIVATE xrx_all)
