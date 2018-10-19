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

#include "viewerLocation.h"

#include <FnAttribute/FnGroupBuilder.h>

#include <iostream>
#include <sstream>
#include <algorithm>

/**** ViewerLocationTree ****/

ViewerLocationTree::ViewerLocationTree() : m_root(nullptr) {}

ViewerLocationTree::~ViewerLocationTree() {}

// static Regex m_locationPathRegex 
ViewerLocationTree::Regex ViewerLocationTree::m_locationPathRegex;

ViewerLocation* ViewerLocationTree::get(const std::string& path)
{
    return getOrCreate(path, false);
}

ViewerLocation* ViewerLocationTree::addOrUpdate(const std::string& path,
    FnAttribute::GroupAttribute attrs, bool isVirtual,
    const Imath::M44d& localXform, bool isLocalXformAbsolute)
{
    ViewerLocation* location = getOrCreate(path, true);

    if (location)
    {
        location->setAttrs(attrs);
        location->setIsVirtual(isVirtual);
        location->setLocalXform(localXform, isLocalXformAbsolute);
    }

    return location;
}

ViewerLocation* ViewerLocationTree::getRoot()
{
    return m_root;
}

ViewerLocation* ViewerLocationTree::findNearestAncestor(const std::string& path,
    bool ignoreVirtualLocations)
{
    if (!m_root)
    {
        return nullptr;
    }

    std::vector<std::string> pathNames;
    if (!splitPath(path, pathNames))
    {
        // Invalid path
        return nullptr;
    }

    ViewerLocation* location = m_root;
    for (auto it = pathNames.begin() + 1; it != pathNames.end(); ++it)
    {
        ViewerLocation* child = location->getChild(*it);
        if (!child || (ignoreVirtualLocations && child->isVirtual()))
        {
            break;
        }

        location = child;
    }

    return location;
}

ViewerLocation* ViewerLocationTree::getOrCreate(const std::string& path,
    bool createNonExisting)
{
    std::vector<std::string> pathNames;
    if (!splitPath(path, pathNames))
    {
        // Invalid path
        return nullptr;
    }

    if (!m_root)
    {
        if(createNonExisting)
        {
            m_root = new ViewerLocation(nullptr, "root");
        }
        else
        {
            return nullptr;
        }
    }

    ViewerLocation* location = m_root;

    for (auto it = pathNames.begin() + 1; it != pathNames.end(); ++it)
    {
        ViewerLocation* child = location->getChild(*it);

        if (!child)
        {
            if(createNonExisting)
            {
                child = new ViewerLocation(location, *it);
            }
            else
            {
                return nullptr;
            }
        }

        location = child;
    }

    return location;
}

bool ViewerLocationTree::remove(const std::string& path)
{
    ViewerLocation* location = get(path);
    if (!location)
    {
        return false;
    }

    ViewerLocation* parent = location->getParent();

    if (parent)
    {
        std::string name = location->getName();
        return parent->removeChild(name);
    }
    else // root node
    {
        if (location == m_root)
        {
            m_root = nullptr;
        }

        delete location;
    }

    return true;
}

bool ViewerLocationTree::splitPath(const std::string& path,
                               std::vector<std::string>& names)
{
    if (!isValidPath(path))
    {
        return false;
    }

    size_t startPos = 1;
    size_t endPos = 1;
    while ((endPos = path.find_first_of('/', startPos)) != std::string::npos)
    {
        names.push_back( path.substr(startPos, endPos - startPos) );
        startPos = endPos + 1;
    }

    names.push_back( path.substr(startPos) );

    return true;
}

bool ViewerLocationTree::isValidPath(const std::string& path)
{
    return m_locationPathRegex.matches(path);
}



/**** ViewerLocationTree::Regex ****/

ViewerLocationTree::Regex::Regex()
{
    regcomp(&m_regex, "^/root(/[_a-zA-Z0-9.]+)*$", REG_EXTENDED);
}

ViewerLocationTree::Regex::~Regex()
{
    regfree(&m_regex);
}

bool ViewerLocationTree::Regex::matches(const std::string& str)
{
    return regexec(&m_regex, str.c_str(), 0, NULL, 0) == 0;
}



/**** ViewerLocation ****/

ViewerLocation::ViewerLocation(ViewerLocation* parent, const std::string& name)
    : m_parent(parent),
      m_name(name),
      m_isLocalXformAbsolute(false),
      m_data(nullptr),
      m_isSelected(false)
{
    if (parent)
    {
        parent->m_children[name] = this;
    }
}

ViewerLocation::~ViewerLocation()
{
    for (auto childEntry : m_children)
    {
        delete childEntry.second;
    }

    if (m_data)
    {
        delete m_data;
    }
}

std::string ViewerLocation::getName()
{
    return m_name;
}

std::string ViewerLocation::getPath()
{
    ViewerLocation* location = this;
    std::string path = std::string("/") + location->getName();

    while (location->m_parent)
    {
        location = location->m_parent;
        path = std::string("/") + location->getName() + path;
    }

    return path;
}

FnAttribute::GroupAttribute ViewerLocation::getAttrs()
{
    return m_attrs;
}

FnAttribute::Attribute ViewerLocation::getAttr(const std::string& attrName)
{
    if (!m_attrs.isValid())
    {
        return FnAttribute::Attribute(); // invalid attribute
    }

    return m_attrs.getChildByName(attrName);
}

FnAttribute::Attribute ViewerLocation::getGlobalAttr(const std::string& attrName)
{
    ViewerLocation* location = this;

    std::string topLevelAttrName = getTopLevelAttrName(attrName);
    FnAttribute::Attribute attr = getAttr(attrName);

    while (!attr.isValid() && location->m_parent)
    {
        location = location->m_parent;

        // If the top level attribute does not have the group inherit flag set
        // to true, then this attribute is not meant to be inherited.
        if (location->doNotInherit(topLevelAttrName))
        {
            return attr;
        }

        attr = location->getAttr(attrName);
        if (attr.isValid())
        {
            return attr;
        }
    }

    return attr;
}

std::string ViewerLocation::getTopLevelAttrName(const std::string& attrName)
{
    std::stringstream ss;
    for (auto it = attrName.begin(); it != attrName.end(); ++it)
    {
        if (*it == '.' ) break;
        ss << *it;
    }

    return ss.str();
}

bool ViewerLocation::doNotInherit(const std::string& topLevelAttrName)
{
    FnAttribute::GroupAttribute topLevelAttr = getAttr(topLevelAttrName);

    return topLevelAttr.isValid() && !topLevelAttr.getGroupInherit();
}

void ViewerLocation::setAttrs(FnAttribute::GroupAttribute attrs)
{
    m_attrs = attrs;
}

bool ViewerLocation::isVirtual()
{
    return m_isVirtual;
}

void ViewerLocation::setIsVirtual(bool virt)
{
    m_isVirtual = virt;
}

void ViewerLocation::setLocalXform(const Imath::M44d& localXform,
    bool isAbsolute)
{
    m_localXform = localXform;
    m_isLocalXformAbsolute = isAbsolute;
}

Imath::M44d ViewerLocation::getLocalXform()
{
    return m_localXform;
}

Imath::M44d ViewerLocation::getWorldXform()
{
    ViewerLocation* location = this;
    Imath::M44d xform = m_localXform;

    while (location->m_parent && !location->m_isLocalXformAbsolute)
    {
        location = location->m_parent;
        xform = xform * location->m_localXform;
    };

    return xform;
}

bool ViewerLocation::isLocalXformAbsolute()
{
    return m_isLocalXformAbsolute;
}

ViewerLocation* ViewerLocation::getParent()
{
    return m_parent;
}


ViewerLocation* ViewerLocation::getRoot()
{
    ViewerLocation* location = this;

    while (location->m_parent)
    {
        location = location->m_parent;
    }

    return location;
}

void ViewerLocation::getChildNames(std::vector<std::string>& childNames)
{
    childNames.clear();
    for (auto elem : m_children)
    {
        childNames.push_back(elem.first);
    }
}

void ViewerLocation::getChildren(std::vector<ViewerLocation*>& children)
{
    children.clear();
    for (auto elem : m_children)
    {
        children.push_back(elem.second);
    }
}

ViewerLocation* ViewerLocation::getChild(const std::string& childName)
{
    auto it = m_children.find(childName);
    if (it != m_children.end())
    {
        return it->second;
    }

    return nullptr;
}

bool ViewerLocation::removeChild(const std::string& childName)
{
    auto it = m_children.find(childName);
    if (it != m_children.end())
    {
        ViewerLocation* child = it->second;
        m_children.erase(it);
        delete child;
        return true;
    }

    return false;
}

void ViewerLocation::setData(ViewerLocationData* data)
{
    m_data = data;
}

ViewerLocationData* ViewerLocation::getData()
{
    return m_data;
}

bool ViewerLocation::hasData()
{
    return m_data != nullptr;
}


bool ViewerLocation::isSelected()
{
	return m_isSelected;
}

bool ViewerLocation::isAncestorSelected()
{
	ViewerLocation* location = this;

    while (location->m_parent)
    {
        location = location->m_parent;
        if (location->isSelected()) { return true; }
    }

 	return false;
}

void ViewerLocation::setSelected(bool select, bool recursive)
{
	if (recursive)
	{
		setSelectedRecursive(select);
	}
	else
	{
		m_isSelected = select;
	}
}

void ViewerLocation::setSelectedRecursive(bool select)
{
    for (auto elem : m_children)
    {
        elem.second->setSelectedRecursive(select);
    }

    m_isSelected = select;
}
