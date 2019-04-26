set(PXR_PACKAGE vtKatana)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        gf
        tf
        vt
        sdf
        katanaPluginApi

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        array
        traits
        value

    PRIVATE_CLASSES
        internalTraits

    PRIVATE_HEADERS
        internalToVt.h
        internalFromVt.h
)

