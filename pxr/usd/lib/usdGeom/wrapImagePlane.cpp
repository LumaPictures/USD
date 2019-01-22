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
#include "pxr/usd/usdGeom/imagePlane.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateFilenameAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFilenameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateFrameAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateFitAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFitAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateOffsetAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateImageCenterAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateImageCenterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateSizeAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateRotateAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCoverageAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCoverageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2), writeSparsely);
}
        
static UsdAttribute
_CreateCoverageOriginAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCoverageOriginAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2), writeSparsely);
}
        
static UsdAttribute
_CreateUseFrameExtensionAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUseFrameExtensionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateFrameOffsetAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFrameOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateFrameCacheAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFrameCacheAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateWidthAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHeightAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAlphaGainAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAlphaGainAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateSqueezeCorrectionAttr(UsdGeomImagePlane &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSqueezeCorrectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdGeomImagePlane()
{
    typedef UsdGeomImagePlane This;

    class_<This, bases<UsdGeomGprim> >
        cls("ImagePlane");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetFilenameAttr",
             &This::GetFilenameAttr)
        .def("CreateFilenameAttr",
             &_CreateFilenameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFrameAttr",
             &This::GetFrameAttr)
        .def("CreateFrameAttr",
             &_CreateFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFitAttr",
             &This::GetFitAttr)
        .def("CreateFitAttr",
             &_CreateFitAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOffsetAttr",
             &This::GetOffsetAttr)
        .def("CreateOffsetAttr",
             &_CreateOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetImageCenterAttr",
             &This::GetImageCenterAttr)
        .def("CreateImageCenterAttr",
             &_CreateImageCenterAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSizeAttr",
             &This::GetSizeAttr)
        .def("CreateSizeAttr",
             &_CreateSizeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateAttr",
             &This::GetRotateAttr)
        .def("CreateRotateAttr",
             &_CreateRotateAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCoverageAttr",
             &This::GetCoverageAttr)
        .def("CreateCoverageAttr",
             &_CreateCoverageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCoverageOriginAttr",
             &This::GetCoverageOriginAttr)
        .def("CreateCoverageOriginAttr",
             &_CreateCoverageOriginAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUseFrameExtensionAttr",
             &This::GetUseFrameExtensionAttr)
        .def("CreateUseFrameExtensionAttr",
             &_CreateUseFrameExtensionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFrameOffsetAttr",
             &This::GetFrameOffsetAttr)
        .def("CreateFrameOffsetAttr",
             &_CreateFrameOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFrameCacheAttr",
             &This::GetFrameCacheAttr)
        .def("CreateFrameCacheAttr",
             &_CreateFrameCacheAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetWidthAttr",
             &This::GetWidthAttr)
        .def("CreateWidthAttr",
             &_CreateWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHeightAttr",
             &This::GetHeightAttr)
        .def("CreateHeightAttr",
             &_CreateHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAlphaGainAttr",
             &This::GetAlphaGainAttr)
        .def("CreateAlphaGainAttr",
             &_CreateAlphaGainAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthAttr",
             &This::GetDepthAttr)
        .def("CreateDepthAttr",
             &_CreateDepthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSqueezeCorrectionAttr",
             &This::GetSqueezeCorrectionAttr)
        .def("CreateSqueezeCorrectionAttr",
             &_CreateSqueezeCorrectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetCameraRel",
             &This::GetCameraRel)
        .def("CreateCameraRel",
             &This::CreateCameraRel)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

WRAP_CUSTOM {
}

} // anonymous namespace

