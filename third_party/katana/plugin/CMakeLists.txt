add_subdirectory(pxrUsdIn)
add_subdirectory(pxrUsdInPrman)
add_subdirectory(pxrUsdInShipped)
add_subdirectory(vmp_usd)

if (KATANA_API_VIEWER_API_VERSION GREATER_EQUAL 3)
    add_subdirectory(viewer_hydra)
endif()
