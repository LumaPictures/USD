//
// Copyright 2018 Pixar
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
#ifndef USDCOMPONENT_H_
#define USDCOMPONENT_H_

#include "usdLocationData.h"
#include "hydraKatana.h"
#include "hydraComponent.h"

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usd/usd/stage.h"

#include <FnViewer/plugin/FnViewerDelegateComponent.h>
#include <FnViewer/plugin/FnViewerDelegate.h>
#include <FnViewer/plugin/FnMathTypes.h>
#include <FnLogging/FnLogging.h>

#include <ImathBox.h>

#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace Foundry::Katana::ViewerAPI;

// A set of rprim paths and their corresponsing instance index. This identifies
// an RPrim with or without instancing. The instance index will be used to find
// the exact UsdPrim for this RPrim for the cases in which the Rprim is an
// instance, otherwise only the SdfPath will be used to identify the UsdPrim and
// the instanceIndex will be ignored.
typedef std::pair<SdfPath, int> RPrimPathAndIndex;
typedef std::set<RPrimPathAndIndex> RPrimsSet;

/**
 * @brief ViewerDelegateComponent used to render USD locations.
 *
 * This class has a reference to the HydraComponent that holds an instance of
 * HydraKatana in order to populate its Render Index with Prims from usd stages.
 * This detects locations of the type "usd" and populate the Render Index with
 * the corresponding usd stage. The stages will be read from the UsdKatanaCache
 * singleton so that they can be reused by and from Katana's other plugins (the
 * PxrUsdIn node, for example). See the USDLocationData class (usdLocation.h),
 * which is the ViewerLocationData implementation for locations of the type usd.
 *
 * Usd prims that can be rendered, when added to a Render Index via a
 * UsdImagingDelegate become RPrims. All the usd related RPrims will be added to
 * the Render Index under a SdfPath root/namespace (SDF_PATH_ROOT) under which
 * all the USD stages loaded by this VDC place their RPrims.
 *
 * Katana USD locations will have a corresponding RPrim path that differs only
 * in the fact that the RPrimpath will be prefixed with SDF_PATH_ROOT. RPrims
 * loaded by an usd location will have a path of this form:
 *   /[SDF_PATH_ROOT]/[LOCATION_PATH]/[STAGE_PRIM_PATH]
 *
 * A stage will load only the prims under the ReferencePath (as defined by the
 * "referencePath" on the usd location), but their full path will be used to the
 * their corresponding RPrim path. For example, if a usd location "/my/location"
 * loads a stage that contains an prim called "/a/b/c/d" and the reference path
 * set to /a/b, then the resulting RPrim path will be:
 *   /[SDF_PATH_ROOT]/my/location/a/b/c/d
 * In this case any prim under /a that is not under /a/b will not be loaded,
 * since it is outside the scope of the reference path.
 *
 * This VDC and its companion USDLayer allow the selection of rendered RPrims.
 * They implement two selection modes, driven by the m_useRprimSelection flag: 
 *
 *  - RPrim Selection: in which RPrims of loaded stages can be highlighted, 
 *    framed by the F key and dragged out of the Viewer. In this mode the
 *    locations that correspond to the selected RPrims that are under a proxy
 *    will be selected in Katana (albeit not being expanded at that moment).
 *    This is called "Sub-Proxy Selection" in the UI. This happens when
 *    m_useRprimSelection == false.
 *
 *  - USD Location Selection: in which the location of the type "usd" (proxy or
 *    non-proxy) that loaded the picked RPrim will be selected. This is a more
 *    standard Katana behaviour. This happens when m_useRprimSelection == true.
 *
 * XXX: Since Foundry did not provide yet a way of framing the camera when the
 *      F key is pressed on an arbitrary bounding box (it only frames on
 *      existing locations that the Viewer can see that contain the "bound"
 *      attribute), we use a ficticious location created by a Viewer terminal op
 *      (RPRIM_BOUNDS_LOCATION_PATH) where the bounds of the selected RPrims
 *      are written in its "bound" attribute. This location is always selected
 *      when in RPrim Selection mode, so that the user can frame the camera on
 *      the selected RPrims, which do not correspond to locations that the
 *      Viewer is aware of.
 */
class USDComponent : public ViewerDelegateComponent
{

public: /* Functions that extend ViewerDelegateComponent */

    /** @brief Constructor. */
    USDComponent();

    /** @brief Destructor. */
    virtual ~USDComponent();

    /** @brief Factory Method. */
    static Foundry::Katana::ViewerAPI::ViewerDelegateComponent* create();

    /** @brief Flushes caches */
    static void flush();

    /** @brief Initializes the ViewportDelegateComponent's resources. */
    virtual void setup() override {}

    /** @brief Cleans up the ViewportDelegateComponent's resources. */
    virtual void cleanup() override {}

    /**
     * @brief Called when a location is created/updated/removed.
     *
     * For locations of the type 'usd', this will load or unload the stage into
     * the HydraKatana's Render Index.
     */
    bool locationEvent(const ViewerLocationEvent& event, bool locationHandled)
        override;
    
     /**
     * @brief Called when the location selection changes in Katana.
     *
     * This intercepts the selection of locations picked by USDLayer. These are
     * prefixed by the SDF_PATH_ROOT namespace. This is necessary for the
     * sub-stage picking/selection. These location paths are considered not
     * valid by HydraComponent::locationsSelected() because they do not start
     * with "/root" and consequently discarded by HydraComponent.
     */
    void locationsSelected(const std::vector<std::string>& locationPaths)
        override;

    /**
     * @brief: Gets the bounding box for USD locations.
     *
     * This only returns the bounds for USD type locaitons. This will not return
     * bounds for sub-proxy RPrims, since these do not correspond to expanded
     * locations, and this Viewer API call is only relevant for cooked
     * locations.
     */
    FnAttribute::DoubleAttribute getBounds(const std::string& locationPath)
        override;

    /** 
     * @brief Gets the "SelectRprims" bool option. 
     *
     * Returns an IntAttribute set to 1 if the  RPrim Selection mode is on and 0
     * if the USD Location Selection mode is on.
     */
    FnAttribute::Attribute getOption(OptionIdGenerator::value_type optionId)
        override;

    /** 
     * @brief Sets the "SelectRprims" bool option. 
     *
     * Receices an IntAttribute set to 1 in order to turn on the RPrim Selection
     * mode and 0 for the USD Location Selection mode.
     */
    void setOption(OptionIdGenerator::value_type optionId,
                   FnAttribute::Attribute attr) override;


public: /* Public functions specific to USDComponent */

    /** @brief: returns true if there are selected RPrims. */
    bool hasSelectedRPrims();

    /** 
     * @brief Gets the locations to be selected for the given sdfPaths.
     *
     * Called by USDLayer when the picking detects a set of SdfPaths.
     *
     * If RPrim selection mode is on, updates the selected RPrims with the ones
     * in sdfPaths, taking into account  the shift/ctrl keyboard modifier (which
     * will define the add/remove selection logic). The returned locations will
     * be the expanded locations for all the RPrims in sdfPaths that belong to a
     * proxy, since non-proxy usd locations' RPrims should only be used for
     * framing the camera, but not for unexpaned location selection.
     *
     * If RPrim selection mode is off then returns the USD locations that loaded
     * the RPrims that correspond to the sdfPaths.
     *
     * This will also update the bounds for the selected RPrims.
     */
    void getLocationsForSelection(const RPrimsSet& rprims,
                                  std::set<std::string>& locationPaths);

    /** 
     * @brief: Updates the selected RPrims, their bounds and highlighting.
     *
     * In USD location selection mode (non sub proxy selection) this will simply
     * clear the RPrim selection, bounds and highlighting.
          */
    void updateSelectedRPrims(const RPrimsSet& rprims,
                              bool shift, bool ctrl);

    /// A root Sdf Path for all the objects in the Render Index. All the roots
    /// for each usd file will be under this global root.
    static const std::string SDF_PATH_ROOT;

    /// A fictitious location used to store the bbox of selected sub-proxy
    /// USD RPrims, to allow these to be framed by the F key. This is added
    /// by a viewer terminal StaticSceneCreate op.
    ///
    /// XXX: See XXX above in this class' comments.
    static const std::string RPRIM_BOUNDS_LOCATION_PATH;

private:
    
    /// Reference to the HydraComponent that holds the HydraKatana instance.
    HydraComponent* m_hydraComponent;

    /// Reference to the HydraKatana, to avoid having to repeat the call to
    /// m_hydraComponent->getHydraKatana()
    HydraKatanaPtr m_hydraKatana;
    
    /// Reference to the location tree from HydraComponent, to avoid having to
    /// repeat the call to m_hydraComponent->getTree()
    ViewerLocationTree* m_tree;

    /// Flag controlled by the "SelectRprims" option. If true the selection mode
    /// is RPrim Selection, otherwise it is USD Location mode.
    bool m_useRprimSelection;

    /// Set of all the sub-stage selected RPrims, used in RPrim selection mode
    RPrimsSet m_selectedRPrims;

    /// Attribute that caches the selected RPrims' total bounding box
    FnAttribute::DoubleAttribute m_selectedRPrimsBounds;


    /** @brief Gets a reference to the HydraComponent, if it wasn't done yet.*/
    bool initHydraComponentRef();

    /** @brief Forces the associated Viewports to redraw. */
    void setViewportsDirty();

    /**
     * @brief Loads a usd stage into HydraKatana's Render Index.
     *
     * Tries to load the stage via UsdKatanaCache for the given location and 
     * its attributes and populates the HydraKatana's Render Index with it.
     * Returns false if it fails to do so.
     */
    bool loadUsd(ViewerLocation* location);

    /** @brief Updates the xform for all USD stages under the given location. */
    void propagateXform(ViewerLocation* location);

    /** @brief Set the selection color according to the selection mode */
    void updateSelectionColor();

    /** @brief Marks the given RPrims and its descendants to be highlighted.*/
    void highlightRprims(const RPrimsSet& rprims, bool replace=true);

    /**
     * @brief Gets the equivalent RPrim SdfPath for the given location path.
     *
     * This prepends the location with the SDF_PATH_ROOT namespace.
     */
    SdfPath locationPathToRPrimSdfPath(const std::string& locationPath);

    /**
     * @brief Gets the location path for the given SdfPath.
     *
     * This removes the SDF_PATH_ROOT prefix namespace.
     */
    std::string rprimSdfPathToLocationPath(const SdfPath& rprimSdfPath);

    /**
     * @brief Gets the USD location that loaded a given RPrim path.
     *
     * For a USD RPrim path, which is prefixed by the SDF_PATH_ROOT namespace,
     * this returns the usd location that loaded it.
     */
    ViewerLocation* getUSDLocationOfRPrimPath(const SdfPath& rprimSdfPath);

    /** @brief Gets all the USD locations below the given location. */
    void getAllUsdDescendants(ViewerLocation* location,
        std::vector<ViewerLocation*>* usdLocations);

    /** @brief Gets the first USD location above the given location. */
    ViewerLocation* findNearestUSDAncestor(const std::string& locationPath);

    /** @brief Gets the first USD location above the given prim SdfPath. */
    ViewerLocation* findNearestUSDAncestor(const SdfPath& sdfPath); 

    /** @brief Applies the usd location's world xform to a bbox. */
    /** @brief: Converts an Imath::Box3d to a DoubleAttribute */
    FnAttribute::DoubleAttribute convertBoundsToAttr(const Imath::Box3d& bbox);

    /**
     *@brief Gets the bounding box that contains all the selected RPrims. 
     *
     * If sub-stage selection is used, this allows USDLayer to, for example,
     * frame the camera on the selected RPrims. The standard CameraControl layer
     * is only able to frame the camera on the whole selected locations.
     */
    void updateSelectedRPrimBounds();
};


#endif
