//
// Copyright 2017 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/base/gf/math.h"

#include <vector>
#include <array>

PXR_NAMESPACE_USING_DIRECTIVE

// No initializer list constructor! This is for making things more readable.
template <typename T> VtArray<T>
_convertArray(const std::vector<T>& v)
{
    VtArray<T> ret;
    ret.reserve(v.size());
    for (const auto& e: v) {
        ret.push_back(e);
    }
    return ret;
}

bool
_verifyArrays(const VtVec3fArray& v1, const VtVec3fArray& v2, double epsilon = 1e-3)
{
    const auto count = v1.size();
    if (count != v2.size()) { return false; }
    for (auto i = decltype(count){0}; i < count; ++i) {
        if (!GfIsClose(v1[i], v2[i], epsilon)) {
            return false;
        }
    }
    return true;
}

template <typename T, typename H> VtArray<T>
_addArrays(const VtArray<T>& v1, const VtArray<T>& v2, H value) {
    VtArray<T> ret;
    const auto count = v1.size();
    ret.resize(count);
    for (auto i = decltype(count){0}; i < count; ++i) {
        ret[i] = v1[i] + v2[i] * value;
    }
    return ret;
}

constexpr auto framesPerSecond = 24.0;

const SdfPath pointsPath("/points");

constexpr double d10 = 1.0;
constexpr double d20 = 2.0;
constexpr double d30 = 3.0;
constexpr double d02 = 0.2;
constexpr double d04 = 0.4;
constexpr double d08 = 0.8;

UsdTimeCode frame1(d10);
UsdTimeCode frame12(d10 + d02);
UsdTimeCode frame14(d10 + d04);
UsdTimeCode frame2(d20);
UsdTimeCode frame22(d20 + d02);
UsdTimeCode frame28(d20 + d08);
UsdTimeCode frame3(d30);

const auto positions1 = _convertArray<GfVec3f>({GfVec3f(1.0f, 2.0f, 3.0f)});
const auto positions2 = _convertArray<GfVec3f>({GfVec3f(1.0f, 2.0f, 3.0f), GfVec3f(6.0f, 3.0f, 2.0f)});
const auto positions3 = _convertArray<GfVec3f>({GfVec3f(8.0f, 3.0f, 5.0f),
                                                GfVec3f(-4.0f, -5.0f, 17.0f), GfVec3f(-4.0f, 23.0f, 12.0f)});

const auto velocities1 = _convertArray<GfVec3f>({GfVec3f(10.0f, 10.0f, 10.0f)});
const auto velocities2 = _convertArray<GfVec3f>({GfVec3f(4.0f, 12.0f, 75.0f), GfVec3f(-4.0f, -83.0f, 65.0f)});
const auto velocities3 = _convertArray<GfVec3f>({GfVec3f(-8.0f, 13.0f, -5.0f),
                                                 GfVec3f(-24.0f, -45.0f, 17.0f), GfVec3f(-44.0f, 23.0f, 112.0f)});

void TestUsdGeomPointsComputePositions()
{
    auto stage = UsdStage::CreateInMemory("test.usda");
    stage->SetFramesPerSecond(framesPerSecond);

    auto points = UsdGeomPoints::Define(stage, pointsPath);

    TF_VERIFY(points,
              "Failed to create prim at %s",
              pointsPath.GetText());

    // Testing nullptr results.
    std::vector<UsdTimeCode> samples1 = {frame14};
    TF_VERIFY(points.ComputePositionsAtTimes(nullptr, samples1, frame2, 1.0f) == 0);

    // Testing empty positions on prim
    std::vector<VtVec3fArray> results1(1);
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame2, 1.0f) == 0);

    // Testing verify function
    TF_VERIFY(_verifyArrays(positions1, positions1));
    TF_VERIFY(!_verifyArrays(positions1, positions2));
    TF_VERIFY(!_verifyArrays(positions1, velocities1));

    points.GetPointsAttr().Set(positions1, frame1);
    points.GetPointsAttr().Set(positions2, frame2);
    points.GetPointsAttr().Set(positions3, frame3);

    // Fall back querying the positions using the built-in interpolation function
    samples1[0] = frame14;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame1, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results1[0], positions1));
    samples1[0] = frame28;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame2, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results1[0], positions2));

    std::vector<VtVec3fArray> results2(2);
    std::vector<UsdTimeCode> samples2 = {frame28, frame12};

    // Inconsistent vector lengths, we should only get one sample.
    TF_VERIFY(points.ComputePositionsAtTimes(&results2, samples2, frame2, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results2[0], positions2));

    // Points count are consistent
    samples2[0] = frame12;
    samples2[1] = frame14;
    TF_VERIFY(points.ComputePositionsAtTimes(&results2, samples2, frame2, 1.0f) == 2);
    TF_VERIFY(_verifyArrays(results2[0], positions1));
    TF_VERIFY(_verifyArrays(results2[1], positions1));

    points.GetVelocitiesAttr().Set(velocities1, frame1);

    // Fall back to interpolation when there are not enough samples for velocity
    samples1[0] = frame28;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame2, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results1[0], positions2));

    VtVec3fArray velocity;
    // Test interpolation with partial velocity values and velocity output
    samples1[0] = frame14;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame1, 1.0f, &velocity) == 1);
    TF_VERIFY(_verifyArrays(results1[0], _addArrays(positions1, velocities1, d04 / framesPerSecond)));
    TF_VERIFY(_verifyArrays(velocity, velocities1));

    // Test interpolation with scale
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame1, d08) == 1);
    TF_VERIFY(_verifyArrays(results1[0], _addArrays(positions1, velocities1, d04 * d08 / framesPerSecond)));

    points.GetVelocitiesAttr().Set(velocities2, frame2);
    points.GetVelocitiesAttr().Set(velocities3, frame3);

    // Making sure still the right value is used
    samples1[0] = frame22;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame2, d08) == 1);
    TF_VERIFY(_verifyArrays(results1[0], _addArrays(positions2, velocities2, d02 * d08 / framesPerSecond)));

    // Reverse interpolation
    samples1[0] = frame12;
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame2, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results1[0], _addArrays(positions2, velocities2, (d10 + d02 - d20) / framesPerSecond)));

    // Outside range
    samples1[0] = UsdTimeCode(d30 + d04);
    TF_VERIFY(points.ComputePositionsAtTimes(&results1, samples1, frame3, 1.0f) == 1);
    TF_VERIFY(_verifyArrays(results1[0], _addArrays(positions3, velocities3, d04 / framesPerSecond)));

    // Two samples
    samples2[0] = frame12;
    samples2[1] = frame28;
    TF_VERIFY(points.ComputePositionsAtTimes(&results2, samples2, frame2, 1.0f) == 2);
    TF_VERIFY(_verifyArrays(results2[0], _addArrays(positions2, velocities2, -d08 / framesPerSecond)));
    TF_VERIFY(_verifyArrays(results2[1], _addArrays(positions2, velocities2, d08 / framesPerSecond)));
}

int main()
{
    TestUsdGeomPointsComputePositions();
}
