# NOTE: This plugin has a really weird side-effect that I wasn't able to track
# down: basically, if it is loaded before the `pxrUsdIn` library, any PxrUsdIn
# node will subsequently crash Katana on cook. I assume this is somehow related
# to a difference in the library load order, but I haven't done an exhaustive
# analysis using LD_DEBUG or anything. Naming this plugin so that it sorts
# after `pxrUsdIn` alphanumerically avoids the crash, so I'm just using the same
# name Pixar does.
set(PXR_PACKAGE viewer_hydra)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        katanaPluginApi
        arch
        glf
        hd
        hdSt
        hdx
        tf
        usd
        usdGeom
        usdImaging
        usdImagingGL
        usdKatana

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${OPENEXR_INCLUDE_DIRS}

    CPPFILES
        plugins.cpp

        hydraComponent.cpp
        hydraKatana.cpp
        hydraLayer.cpp
        usdComponent.cpp
        usdLayer.cpp
        usdLocationData.cpp
        viewerUtils/viewerLocation.cpp

        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnEventWrapper.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnManipulator.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnManipulatorHandle.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnOptionIdGenerator.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnViewerDelegate.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnViewerDelegateComponent.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnViewport.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnViewportCamera.cpp
        ${KATANA_API_SOURCE_DIR}/FnViewer/plugin/FnViewportLayer.cpp
)
