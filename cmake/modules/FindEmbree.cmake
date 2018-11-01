#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
#=============================================================================
#
# The module defines the following variables:
#   EMBREE_INCLUDE_DIR - path to embree header directory
#   EMBREE_LIBRARY     - path to embree library file
#       EMBREE_FOUND   - true if embree was found
#
# Example usage:
#   find_package(EMBREE)
#   if(EMBREE_FOUND)
#     message("EMBREE found: ${EMBREE_LIBRARY}")
#   endif()
#
#=============================================================================

# Embree 2 lib name has no version suffix
foreach (_EMBREE_VERSION_SUFFIX "" "3")
    find_library(EMBREE_LIBRARY
            "embree${_EMBREE_VERSION_SUFFIX}"
        HINTS
            "${EMBREE_LOCATION}/lib64"
            "${EMBREE_LOCATION}/lib"
            "$ENV{EMBREE_LOCATION}/lib64"
            "$ENV{EMBREE_LOCATION}/lib"
        DOC
            "Embree library path"
    )

    if (EMBREE_LIBRARY)
        break()
    endif ()
endforeach ()

foreach (_EMBREE_VERSION_SUFFIX "2" "3")
    find_path(EMBREE_INCLUDE_DIR
        embree${_EMBREE_VERSION_SUFFIX}/rtcore.h
    HINTS
        "${EMBREE_LOCATION}/include"
        "$ENV{EMBREE_LOCATION}/include"
    DOC
        "Embree headers path"
    )

    if (EMBREE_INCLUDE_DIR AND EXISTS "${EMBREE_INCLUDE_DIR}/embree${_EMBREE_VERSION_SUFFIX}/rtcore_version.h" )
        foreach (_EMBREE_VERSION_COMP "MAJOR" "MINOR" "PATCH")
            file(STRINGS "${EMBREE_INCLUDE_DIR}/embree${_EMBREE_VERSION_SUFFIX}/rtcore_version.h" _EMBREE_TMP REGEX "^#define RTC(ORE)?_VERSION_${_EMBREE_VERSION_COMP} .*$")
            string(REGEX MATCHALL "[0-9]+" "EMBREE_VERSION_${_EMBREE_VERSION_COMP}" ${_EMBREE_TMP})
        endforeach ()
        set(EMBREE_VERSION ${EMBREE_VERSION_MAJOR}.${EMBREE_VERSION_MINOR}.${EMBREE_VERSION_PATCH})
        break()
    endif ()
endforeach ()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Embree
    REQUIRED_VARS
        EMBREE_INCLUDE_DIR
        EMBREE_LIBRARY
        EMBREE_VERSION_MAJOR
        EMBREE_VERSION_MINOR
        EMBREE_VERSION_PATCH
    VERSION_VAR
        EMBREE_VERSION
)
