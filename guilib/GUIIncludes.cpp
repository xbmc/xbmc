/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIIncludes.h"
#include "addons/Skin.h"
#include "utils/GUIInfoManager.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"

using namespace std;

CGUIIncludes::CGUIIncludes()
{
}

CGUIIncludes::~CGUIIncludes()
{
}

void CGUIIncludes::ClearIncludes()
{
  m_includes.clear();
  m_defaults.clear();
  m_constants.clear();
  m_files.clear();
}

bool CGUIIncludes::LoadIncludes(const CStdString &includeFile)
{
  // check to see if we already have this loaded
  if (HasIncludeFile(includeFile))
    return true;

  TiXmlDocument doc;
  if (!doc.LoadFile(includeFile))
  {
    CLog::Log(LOGINFO, "Error loading includes.xml file (%s): %s (row=%i, col=%i)", includeFile.c_str(), doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
    return false;
  }
  // success, load the tags
  if (LoadIncludesFromXML(doc.RootElement()))
  {
    m_files.push_back(includeFile);
    return true;
  }
  return false;
}

bool CGUIIncludes::LoadIncludesFromXML(const TiXmlElement *root)
{
  if (!root || strcmpi(root->Value(), "includes"))
  {
    CLog::Log(LOGERROR, "Skin includes must start with the <includes> tag");
    return false;
  }
  const TiXmlElement* node = root->FirstChildElement("include");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      CStdString tagName = node->Attribute("name");
      m_includes.insert(pair<CStdString, TiXmlElement>(tagName, *node));
    }
    else if (node->Attribute("file"))
    { // load this file in as well
      LoadIncludes(g_SkinInfo->GetSkinPath(node->Attribute("file")));
    }
    node = node->NextSiblingElement("include");
  }
  // now defaults
  node = root->FirstChildElement("default");
  while (node)
  {
    if (node->Attribute("type") && node->FirstChild())
    {
      CStdString tagName = node->Attribute("type");
      m_defaults.insert(pair<CStdString, TiXmlElement>(tagName, *node));
    }
    node = node->NextSiblingElement("default");
  }
  // and finally constants
  node = root->FirstChildElement("constant");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      CStdString tagName = node->Attribute("name");
      m_constants.insert(pair<CStdString, float>(tagName, (float)atof(node->FirstChild()->Value())));
    }
    node = node->NextSiblingElement("constant");
  }
  return true;
}

bool CGUIIncludes::HasIncludeFile(const CStdString &file) const
{
  for (iFiles it = m_files.begin(); it != m_files.end(); ++it)
    if (*it == file) return true;
  return false;
}

void CGUIIncludes::ResolveIncludes(TiXmlElement *node, const CStdString &type)
{
  // we have a node, find any <include file="fileName">tagName</include> tags and replace
  // recursively with their real includes
  if (!node) return;

  // First add the defaults if this is for a control
  if (!type.IsEmpty())
  { // resolve defaults
    map<CStdString, TiXmlElement>::const_iterator it = m_defaults.find(type);
    if (it != m_defaults.end())
    {
      const TiXmlElement &element = (*it).second;
      const TiXmlElement *tag = element.FirstChildElement();
      while (tag)
      {
        // we insert at the end of block
        node->InsertEndChild(*tag);
        tag = tag->NextSiblingElement();
      }
    }
  }
  TiXmlElement *include = node->FirstChildElement("include");
  while (include && include->FirstChild())
  {
    // have an include tag - grab it's tag name and replace it with the real tag contents
    const char *file = include->Attribute("file");
    if (file)
    { // we need to load this include from the alternative file
      LoadIncludes(g_SkinInfo->GetSkinPath(file));
    }
    const char *condition = include->Attribute("condition");
    if (condition)
    { // check this condition
      if (!g_infoManager.GetBool(g_infoManager.TranslateString(condition)))
      {
        include = include->NextSiblingElement("include");
        continue;
      }
    }
    CStdString tagName = include->FirstChild()->Value();
    map<CStdString, TiXmlElement>::const_iterator it = m_includes.find(tagName);
    if (it != m_includes.end())
    { // found the tag(s) to include - let's replace it
      const TiXmlElement &element = (*it).second;
      const TiXmlElement *tag = element.FirstChildElement();
      while (tag)
      {
        // we insert before the <include> element to keep the correct
        // order (we render in the order given in the xml file)
        node->InsertBeforeChild(include, *tag);
        tag = tag->NextSiblingElement();
      }
      // remove the <include>tagName</include> element
      node->RemoveChild(include);
      include = node->FirstChildElement("include");
    }
    else
    { // invalid include
      CLog::Log(LOGWARNING, "Skin has invalid include: %s", tagName.c_str());
      include = include->NextSiblingElement("include");
    }
  }
}

bool CGUIIncludes::ResolveConstant(const CStdString &constant, float &value) const
{
  map<CStdString, float>::const_iterator it = m_constants.find(constant);
  if (it == m_constants.end())
    value = (float)atof(constant.c_str());
  else
    value = it->second;
  return true;
}

