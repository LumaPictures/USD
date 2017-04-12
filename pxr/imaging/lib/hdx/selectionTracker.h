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
#ifndef HDX_SELECTION_TRACKER_H
#define HDX_SELECTION_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include <boost/smart_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;
class TfToken;
class SdfPath;
class VtValue;

typedef boost::shared_ptr<class HdxSelection> HdxSelectionSharedPtr;
typedef boost::shared_ptr<class HdxSelectionTracker> HdxSelectionTrackerSharedPtr;
typedef boost::weak_ptr<class HdxSelectionTracker> HdxSelectionTrackerWeakPtr;

/// \class HdxSelection
///
/// HdxSelection holds a collection of items which are rprims, instances of
/// rprim, sub elements of rprim (such as faces, verts). HdxSelectionTracker
/// takes HdxSelection and generates GPU buffer to be used for highlighting.
///
class HdxSelection {
public:
    HdxSelection(HdRenderIndex * renderIndex)
        : _renderIndex(renderIndex) { }

    HdRenderIndex * GetRenderIndex() const {
        return _renderIndex;
    }

    // XXX: persisting selection across render index changes is complicated
    // right now, because of the HdRenderIndex pointer.  CopySelection
    // exists as a copy-constructor that ignores the render index.  In
    // the glorious future, we should delete this function and delete
    // the render index member variable.
    void CopySelection(HdxSelection const &other) {
        selectedPrims = other.selectedPrims;
        selectedInstances = other.selectedInstances;
        selectedFaces = other.selectedFaces;
    }

    void AddRprim(SdfPath const &path) {
        selectedPrims.push_back(path);
    }

    void AddInstance(
        SdfPath const &path, VtIntArray const &instanceIndex=VtIntArray()) {
        selectedPrims.push_back(path);
        selectedInstances[path].push_back(instanceIndex);
    }

    void AddFaces(
        SdfPath const &path, VtIntArray const &faceIndices) {
        selectedPrims.push_back(path);
        selectedFaces[path] = faceIndices;
    }

    // TODO: encapsulate members

    typedef TfHashMap<SdfPath, std::vector<VtIntArray>, SdfPath::Hash> InstanceMap;
    typedef TfHashMap<SdfPath, VtIntArray, SdfPath::Hash> ElementMap;

    // The SdfPaths are expected to be resolved rprim paths,
    // root paths will not be expanded.
    // Duplicated entries are allowed.
    SdfPathVector selectedPrims;

    /// This maps from prototype path to a vector of instance indices which is
    /// also a vector (because of nested instancing).
    InstanceMap selectedInstances;

    // The selected elements (faces, points, edges) , if any, for the selected
    // objects. This maps from object path to a vector of element indices.
    ElementMap selectedFaces;

private:
    HdRenderIndex * _renderIndex;
};

/// \class HdxSelectionTracker
///
/// HdxSelectionTracker is a base class for observing selection state and
/// providing selection highlighting details to interested clients.
///
class HdxSelectionTracker {
public:
    HDX_API
    HdxSelectionTracker();
    virtual ~HdxSelectionTracker() = default;

    /// Update dirty bits in the ChangeTracker and compute required primvars for
    /// later consumption.
    HDX_API
    virtual void Sync(HdRenderIndex* index);

    /// Populates an array of offsets required for selection highlighting.
    /// Returns true if offsets has anything selected.
    HDX_API
    virtual bool GetBuffers(HdRenderIndex const* index,
                            VtIntArray* offsets) const;

    /// Returns a monotonically increasing version number, which increments
    /// whenever the result of GetBuffers has changed. Note that this number may
    /// overflow and become negative, thus clients should use a not-equal
    /// comparison.
    HDX_API
    int GetVersion() const;

    void SetSelection(HdxSelectionSharedPtr const &selection) {
        _selection = selection;
        _IncrementVersion();
    }

    HdxSelectionSharedPtr const &GetSelectionMap() const {
        return _selection;
    }

protected:
    /// Increments the internal selection state version, used for invalidation
    /// via GetVersion().
    HDX_API
    void _IncrementVersion();

private:
    int _version;
    HdxSelectionSharedPtr _selection;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_SELECTION_TRACKER_H
