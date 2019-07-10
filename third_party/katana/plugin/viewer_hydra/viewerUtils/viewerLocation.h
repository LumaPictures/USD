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

#ifndef VIEWER_LOCATION_H
#define VIEWER_LOCATION_H

#include <FnAttribute/FnAttribute.h>

#include <ImathMatrix.h>

#include <map>
#include <vector>
#include <memory>
#include <regex.h>

// Forward declaration
class ViewerLocation;


/**
 * @brief Represents a loation tree data structure.
 *
 * This class contains a whole tree of ViewerLocation nodes. It contains a
 * single root location ("/root"). This also presents functions that allow the
 * access to the tree using location path strings, as well as location path
 * utility functions. A possible use of an instance of this class is as the
 * location data structure kept by one or more ViewerDelegateComponent and
 * consumed by one or more ViewportLayer.
 *
 * A location path is only considered valid if it starts with /root and each
 * location name contains only alphanumeric characters or underscore ('_') and
 * are separated by forward slashes ('/'). Examples
 *    /root - valid
 *    /root/world/geo/my_location123 - valid
 *    /root/a/b/c/ - invalid (invalid final forward slash)
 *    /root/a-b-c  - invalid (contains the '-' invalid character)
 *    /my_root/world/geo/my_location123 - invalid (doesn't start with /root)
 */
class ViewerLocationTree
{
public:
    /** @brief Constructor. */
    ViewerLocationTree();

    /** @brief Destructor. */
    ~ViewerLocationTree();

    /**
     * @brief Adds or updates a location with the given path.
     *
     * This will construct all the locations that do not exist above the
     * location if necessary and will return the location if it already exists.
     * Either way, the Attributes, virtual (proxy) flag and local xform will
     * be set in the location. Returns null if the location path is an invalid
     * string.
     */
    ViewerLocation* addOrUpdate(const std::string& path,
                                FnAttribute::GroupAttribute attrs,
                                bool isVirtual,
                                const Imath::M44d& localXform,
                                bool isLocalXformAbsolute);

    /**
     * @brief Gets a location with the given path. 
     *
     * Returns null if the location doesn't exist o the path is invalid.
     */
    ViewerLocation* get(const std::string& path);

    /** @brief Gets the root location if it was created already*/
    ViewerLocation* getRoot();

    /**
     * @brief Gets the nearest existing ancestor above the given path.
     *
     * The given path has to be valid, but it doesn't need to exist in the tree.
     * This will return the first ancestor that exists in the tree, starting on
     * the given location and walking up. If the location of the given path
     * exists in the tree, then it will be returned.
     *
     * @param path: A valid location path.
     * @param ignoreVirtualLocations: Ignore any ancestor that is virtual. This
     *      might be useful to find which ancestor above a proxy location is
     *      holding the proxy, for example.
     */
    ViewerLocation* findNearestAncestor(const std::string& path,
                                        bool ignoreVirtualLocations=false);

    /**
     * @brief Removes the location with the given path.
     *
     * Returns true if the location existed when the function was called.
     */
    bool remove(const std::string& path);

    /** @brief Splits a valid path into a vector of its location names. */
    bool splitPath(const std::string& path, std::vector<std::string>& names);

    /** @brief Tells if a location path is valid. */
    bool isValidPath(const std::string& path);

private:

    /** @brief Utility function used by get() and add(). */
    ViewerLocation* getOrCreate(const std::string& path,
                                bool createNonExisting);

    /// The root location.
    ViewerLocation* m_root;

    /// A Regex used to check if a location path is valid.
    class Regex
    {
    public:
        Regex();
        ~Regex();
        bool matches(const std::string& str);

    private:
        regex_t m_regex;
    };

    static Regex m_locationPathRegex;
};

/**
 * @brief Abstract class that represents some location data.
 *
 * This class is used by ViewerLocation and must be extended in order to hold
 * per location data. Usually this data is collected/contructed in a Viewer
 * Delegate Component plugin and consumed by a Viewport Layer plugin.
 */
class ViewerLocationData
{
public:
    /** @brief Constructor. */
    ViewerLocationData(ViewerLocation* location) : m_location(location) {}

    /** @brief Destructor. */
    virtual ~ViewerLocationData() {}

    /** @brief Gets the location that hods this data. */
    ViewerLocation* getLocation() { return m_location; }

private:
    /** A pointer to the location that holds this data. */
    ViewerLocation* m_location;
};


/**
 * @brief Tree node that holds Location information for Viewer plugins.
 *
 * This is a tree node that cotains informationa about a location. It contains
 * pointers to the child locations via a map, a pointer to the parent
 * location, some fixed data (virtual/proxy flag, xform, selection flag,
 * location attributes) and some arbitrary data (of the a type that is child of
 * ViewerLocationData).
 */
class ViewerLocation
{
public:

    /** 
     * @brief Constructor.
     *
     * Creates a location node, hanging it under a given parent (which can be
     * null for a root location). This is the only way a location can be added
     * to another parent location.
     * 
     * @param parent: An existing parent location or null (if this is a root)
     * @param name: The name of this location under the parent. Example: if this
     *    location is "/root/world/geo", the parent is the "/root/world" location 
     *    and the name is "geo".
     */
    ViewerLocation(ViewerLocation* parent, const std::string& name);
    
    /** 
     * @brief Destructor.
     *
     * This will guarantee that all its descendants are recursively deleted, as
     * well as the associated data.
     */
    virtual ~ViewerLocation();


    /** @brief Gets the name of the location. Ex: "world" for "/root/world". */
    std::string getName();

    /** @brief Gets the location's full path. Ex: "/root/world/geo". */
    std::string getPath();


    /** @brief: Gets the root location of the current location's tree. */
    ViewerLocation* getRoot();

    /** @brief: Gets the parent location. Null if this is a root location. */
    ViewerLocation* getParent();

    /** @brief: Gets names of the child locations. */
    void getChildNames(std::vector<std::string>& childNames);

    /** @brief: Gets an ordered vector of the child locations. */
    void getChildren(std::vector<ViewerLocation*>& children);

    /** @brief: Gets the child with the given name. Or null if it doesn't exist. */
    ViewerLocation* getChild(const std::string& childName);

    /** @brief Removes and destroyes the child location and its descendants. */
    bool removeChild(const std::string& childName);


    /**
     * @brief Sets the location's Attributes.
     * 
     * The Attributes should be set when the location is cooked.
     */
    void setAttrs(FnAttribute::GroupAttribute attrs);

    /** @brief Gets the location's Attributes. */
    FnAttribute::GroupAttribute getAttrs();

    /** @brief Gets the location's Attribute with the given name. */
    FnAttribute::Attribute getAttr(const std::string& attrName);
    
    /** @brief Gets the global Attribute with the given name. */
    FnAttribute::Attribute getGlobalAttr(const std::string& attrName);

    /**
     * @brief Sets the location's virtual (proxy) flag.
     * 
     * Sets if this location is virtual (a proxy location).
     */
    void setIsVirtual(bool virt);

    /** @brief Tells if the location is virtual (a proxy location). */
    bool isVirtual();


    /**
     * @brief Sets the local xform and if it is absolute. 
     *
     * The xform is absolute if it is not meant to be concatenated to the parent
     * xform.
     */
    void setLocalXform(const Imath::M44d& localXform, bool isAbsolute);
    
    /** @brief Gets the location's local xform. */
    Imath::M44d getLocalXform();
    
    /** @brief Tells if the location's local xform is absolute. */
    bool isLocalXformAbsolute();

    /**
     * @brief Gets the location's world xform.
     *
     * The world xform is calculated by this function by concatenating all the
     * local xform to the xforms of all the ancestors (taking into account the
     * isAbsolute flag of each one).
     */
    Imath::M44d getWorldXform();


    /** @brief Sets arbitrary data on this location. */
    void setData(ViewerLocationData* data);

    /** @brief Gets arbitrary data previously set on this location. */
    ViewerLocationData* getData();
    
    /** @brief Tells if this location has previously set arbitrary data. */
    bool hasData();


    /** @brief Tells if this location is currently selected. */
    bool isSelected();

    /** @brief Tells if any of the ancestors of this location is selected. */
    bool isAncestorSelected();
    
    /** @brief Sets this location as selected. */
    void setSelected(bool select, bool recursive=false);

private:
    /// Pointer to the parent
    ViewerLocation* m_parent;

    /// Children map, indexed by their names
    std::map<std::string, ViewerLocation*> m_children;

    /// The location name
    std::string m_name;

    /// The location Attributes
    FnAttribute::GroupAttribute m_attrs;

    /// The flag that specifies if this location is virtual/proxy.
    bool m_isVirtual;
    
    /// The location's local xform
    Imath::M44d m_localXform;

    /// The flag that specifies if this locations xform is absolute.
    bool m_isLocalXformAbsolute;

    /// This location's data
    ViewerLocationData* m_data;

    /// The flag that specifies if this locations is selected.
    bool m_isSelected;

    /** @brief Recusive function that sets all the descendants' selection. */
    void setSelectedRecursive(bool select);

    /** @brief Gets the top level attr group name of the given full attr name.*/
    std::string getTopLevelAttrName(const std::string& attrName);

    /** @brief Returns if the top level group attr's group inherit is false. */
    bool doNotInherit(const std::string& topLevelAttrName);
};


#endif //VIEWER_LOCATION_H