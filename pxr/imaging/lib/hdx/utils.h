//
// Copyright 2019 Pixar
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
//
#ifndef HDX_UTILS
#define HDX_UTILS

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"

#include "pxr/base/gf/vec2i.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdxUtils
{

/// Query the screen size from either the currently active framebuffer or
/// using the OpenGL viewport size.
HDX_API
GfVec2i GetScreenSize();

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_UTILS
