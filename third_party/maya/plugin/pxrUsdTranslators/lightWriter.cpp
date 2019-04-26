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
#include "pxrUsdTranslators/lightWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/translatorRfMLight.h"
#include "usdMaya/primWriterRegistry.h"

#include "pxr/usd/usdLux/shapingAPI.h"

#include <maya/MFnPointLight.h>
#include <maya/MFnSpotLight.h>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


// The typedef below is needed because PXRUSDMAYA_REGISTER_WRITER
// needs it's second param to be something that can be inserted into
// a c++ name
#define DEFINE_MAYA_LIGHT_WRITER(mayaLightTypeName, MayaLightMFn, UsdLuxType) \
typedef PxrUsdTranslators_LightWriter<MayaLightMFn, UsdLuxType> \
        PxrUsdTranslators_LightWriter_##MayaLightMFn##_##UsdLuxType; \
PXRUSDMAYA_REGISTER_WRITER( \
    mayaLightTypeName, \
    PxrUsdTranslators_LightWriter_##MayaLightMFn##_##UsdLuxType); \
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mayaLightTypeName, UsdLuxType); \
template<> \
bool PxrUsdTranslators_LightWriter<MayaLightMFn, UsdLuxType>::writeLightAttrs( \
        const UsdTimeCode& usdTime, \
        UsdLuxType& primSchema, \
        MayaLightMFn& lightFn)


DEFINE_MAYA_LIGHT_WRITER(pointLight, MFnPointLight, UsdLuxSphereLight)
{
    return writeSphereLightAttrs(usdTime, primSchema, lightFn);
}

DEFINE_MAYA_LIGHT_WRITER(spotLight, MFnSpotLight, UsdLuxSphereLight)
{
    if (!writeSphereLightAttrs(usdTime, primSchema, lightFn)) {
        return false;
    }

    MStatus status;

    auto shapingAPI = UsdLuxShapingAPI::Apply(_usdPrim);

    const double dropOff = lightFn.dropOff(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (dropOff != 0) {
        // Not sure what formula maya uses for "dropOff", so just
        // translating straight for now
        _SetAttribute(shapingAPI.CreateShapingFocusAttr(),
                      static_cast<float>(dropOff), usdTime);
    }

    double coneAngle = GfRadiansToDegrees(lightFn.coneAngle(&status)) * 0.5;
    CHECK_MSTATUS_AND_RETURN(status, false);
    double penumbraAngle = GfRadiansToDegrees(lightFn.penumbraAngle(&status));
    CHECK_MSTATUS_AND_RETURN(status, false);

    float cutoff = static_cast<float>(coneAngle + penumbraAngle);
    float softness = static_cast<float>(
            cutoff == 0 ? 0 : penumbraAngle / cutoff);

    _SetAttribute(shapingAPI.CreateShapingConeAngleAttr(), cutoff, usdTime);
    if (softness != 0)
    {
        _SetAttribute(shapingAPI.CreateShapingConeSoftnessAttr(),
                      softness, usdTime);
    }

    return true;
}


// TODO: move into a plugin
// Renderman specific lights

PXRUSDMAYA_DEFINE_WRITER(PxrAovLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDiskLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDistantLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDomeLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrEnvDayLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrMeshLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrRectLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrSphereLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXR_NAMESPACE_CLOSE_SCOPE
