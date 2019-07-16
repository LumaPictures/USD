pxr_setup_third_party_python_init()

add_subdirectory(plugin)
add_subdirectory(shaders)

if (NOT PXR_BUILD_USD)
    pxr_setup_plugins()
endif()