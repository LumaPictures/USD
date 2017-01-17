//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXRUSDMAYA_TRANSLATOR_UTIL_H
#define PXRUSDMAYA_TRANSLATOR_UTIL_H

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/envSetting.h"

#include <maya/MObject.h>
#include <maya/MString.h>

extern TfEnvSetting<bool> PIXMAYA_DEBUG_USD_ASSEM;

// TODO: determine if the TfEnvSetting check here is performant, and cache if
// not...

/// \brief Print \p msg if the PIXMAYA_DEBUG_USD_ASSEM is enabled, otherwise,
/// do nothing.
#define DEBUG_PRINT(msg) \
{ \
    if( TfGetEnvSetting(PIXMAYA_DEBUG_USD_ASSEM) ) \
    { \
        MString msg_var("PXRUSD: "); \
        msg_var += msg; \
        MGlobal::displayInfo(msg_var); \
        std::cout << msg_var.asChar() << std::endl; \
    } \
} \

/// \brief Provides helper functions for other readers to use.
struct PxrUsdMayaTranslatorUtil
{
    /// \brief Often when creating a prim, we want to first create a Transform
    /// node. This is a small helper to do this. If the \p args provided
    /// indicate that animation should be read, any transform animation from
    /// the prim is transferred onto the Maya transform node. If \p context is
    /// non-NULL, the new Maya node will be registered to the path of
    /// \p usdPrim.
    static bool
    CreateTransformNode(
            const UsdPrim& usdPrim,
            MObject& parentNode,
            const PxrUsdMayaPrimReaderArgs& args,
            PxrUsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a node for \p usdPrim of type \p
    /// nodeTypeName under \p parentNode. If \p context is non-NULL,
    /// the new Maya node will be registered to the path of \p usdPrim.
    static bool
    CreateNode(
            const UsdPrim& usdPrim,
            const MString& nodeTypeName,
            MObject& parentNode,
            PxrUsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a node named \p nodeName of type \p
    /// nodeTypeName under \p parentNode. Note that this version does
    /// NOT take a context and cannot register the newly created Maya node
    /// since it does not know the SdfPath to an originating object.
    static bool
    CreateNode(
            const MString& nodeName,
            const MString& nodeTypeName,
            MObject& parentNode,
            MStatus* status,
            MObject* mayaNodeObj);

    template<typename T>
    static bool
    GetTimeSamples(
            const T& source,
            const PxrUsdMayaPrimReaderArgs& args,
            std::vector<double>* outSamples)
    {
        if (args.HasCustomFrameRange()) {
            std::vector<double> tempSamples;
            source.GetTimeSamples(&tempSamples);
            bool didPushSample = false;
            for (double t : tempSamples) {
                if (t >= args.GetStartTime() && t <= args.GetEndTime()) {
                    outSamples->push_back(t);
                    didPushSample = true;
                }
            }
            return didPushSample;
        } else {
            return source.GetTimeSamples(outSamples);
        }
    }

    /// \brief Helper to get a maya namespace string from a usd path string
    /// \p primPathStr - essentially just replaces '/' with ':'
    static std::string
    GetNamespace(const std::string& primPathStr, bool trailingColon=true);

    /// \brief Helper to get a maya namespace name from a usd path
    /// \p primPath - essentially just replaces '/' with ':'
    static std::string
    GetNamespace(const SdfPath& primPath, bool trailingColon=true);

    /// \brief Helper to get a maya namespace string from the parent of a usd path
    /// string \p primPathStr - essentially just calls
    /// GetNamespace(TfGetPathName(\p primPathStr))
    static std::string
    GetParentNamespace(const std::string& primPathStr, bool trailingColon=true);

    /// \brief Helper to get a maya namespace string from the parent of a usd path
    /// string \p primPath - essentially just calls
    /// GetNamespace(\p primPath.ParentPath())
    static std::string
    GetParentNamespace(const SdfPath& primPath, bool trailingColon=true);

    /// \brief Helper to create a maya namespace recursively, including all
    /// parent namespaces. Returns true if the namespace was successfully created.
    static bool
    CreateNamespace(const std::string& fullNamespace);

    /// \brief Helper to create the parent namespace of the given maya namespace
    /// \p fullNamespace recursively. Returns true if the parent namespace was
    /// successfully created.
    static bool
    CreateParentNamespace(const std::string& fullNamespace);
};


#endif // PXRUSDMAYA_TRANSLATOR_UTIL_H
