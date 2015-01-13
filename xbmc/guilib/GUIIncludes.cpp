/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIIncludes.h"
#include "addons/Skin.h"
#include "GUIInfoManager.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/StringUtils.h"
#include "interfaces/info/SkinVariable.h"

using namespace std;

CGUIIncludes::CGUIIncludes()
{
  m_constantAttributes.insert("x");
  m_constantAttributes.insert("y");
  m_constantAttributes.insert("width");
  m_constantAttributes.insert("height");
  m_constantAttributes.insert("center");
  m_constantAttributes.insert("max");
  m_constantAttributes.insert("min");
  m_constantAttributes.insert("w");
  m_constantAttributes.insert("h");
  m_constantAttributes.insert("time");
  m_constantAttributes.insert("acceleration");
  m_constantAttributes.insert("delay");
  m_constantAttributes.insert("start");
  m_constantAttributes.insert("end");
  m_constantAttributes.insert("center");
  m_constantAttributes.insert("border");
  m_constantAttributes.insert("repeat");
  
  m_constantNodes.insert("posx");
  m_constantNodes.insert("posy");
  m_constantNodes.insert("left");
  m_constantNodes.insert("centerleft");
  m_constantNodes.insert("right");
  m_constantNodes.insert("centerright");
  m_constantNodes.insert("top");
  m_constantNodes.insert("centertop");
  m_constantNodes.insert("bottom");
  m_constantNodes.insert("centerbottom");
  m_constantNodes.insert("width");
  m_constantNodes.insert("height");
  m_constantNodes.insert("offsetx");
  m_constantNodes.insert("offsety");
  m_constantNodes.insert("textoffsetx");
  m_constantNodes.insert("textoffsety");  
  m_constantNodes.insert("textwidth");
  m_constantNodes.insert("spinposx");
  m_constantNodes.insert("spinposy");
  m_constantNodes.insert("spinwidth");
  m_constantNodes.insert("spinheight");
  m_constantNodes.insert("radioposx");
  m_constantNodes.insert("radioposy");
  m_constantNodes.insert("radiowidth");
  m_constantNodes.insert("radioheight");
  m_constantNodes.insert("markwidth");
  m_constantNodes.insert("markheight");
  m_constantNodes.insert("sliderwidth");
  m_constantNodes.insert("sliderheight");
  m_constantNodes.insert("itemgap");
  m_constantNodes.insert("bordersize");
  m_constantNodes.insert("timeperimage");
  m_constantNodes.insert("fadetime");
  m_constantNodes.insert("pauseatend");
}

CGUIIncludes::~CGUIIncludes()
{
}

void CGUIIncludes::ClearIncludes()
{
  m_includes.clear();
  m_defaults.clear();
  m_constants.clear();
  m_skinvariables.clear();
  m_files.clear();
}

bool CGUIIncludes::LoadIncludes(const std::string &includeFile)
{
  // check to see if we already have this loaded
  if (HasIncludeFile(includeFile))
    return true;

  CXBMCTinyXML doc;
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
      std::string tagName = node->Attribute("name");
      m_includes.insert(pair<std::string, TiXmlElement>(tagName, *node));
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
      std::string tagName = node->Attribute("type");
      m_defaults.insert(pair<std::string, TiXmlElement>(tagName, *node));
    }
    node = node->NextSiblingElement("default");
  }
  // and finally constants
  node = root->FirstChildElement("constant");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      std::string tagName = node->Attribute("name");
      m_constants.insert(make_pair(tagName, node->FirstChild()->ValueStr()));
    }
    node = node->NextSiblingElement("constant");
  }

  node = root->FirstChildElement("variable");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      std::string tagName = node->Attribute("name");
      m_skinvariables.insert(make_pair(tagName, *node));
    }
    node = node->NextSiblingElement("variable");
  }

  return true;
}

bool CGUIIncludes::HasIncludeFile(const std::string &file) const
{
  for (iFiles it = m_files.begin(); it != m_files.end(); ++it)
    if (*it == file) return true;
  return false;
}

void CGUIIncludes::ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions /* = NULL */)
{
  if (!node)
    return;
  ResolveIncludesForNode(node, xmlIncludeConditions);

  TiXmlElement *child = node->FirstChildElement();
  while (child)
  {
    ResolveIncludes(child, xmlIncludeConditions);
    child = child->NextSiblingElement();
  }
}

void CGUIIncludes::ResolveIncludesForNode(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions /* = NULL */)
{
  // we have a node, find any <include file="fileName">tagName</include> tags and replace
  // recursively with their real includes
  if (!node) return;

  // First add the defaults if this is for a control
  std::string type;
  if (node->ValueStr() == "control")
  {
    type = XMLUtils::GetAttribute(node, "type");
    map<std::string, TiXmlElement>::const_iterator it = m_defaults.find(type);
    if (it != m_defaults.end())
    {
      // we don't insert <left> et. al. if <posx> or <posy> is specified
      bool hasPosX(node->FirstChild("posx") != NULL);
      bool hasPosY(node->FirstChild("posy") != NULL);

      const TiXmlElement &element = (*it).second;
      const TiXmlElement *tag = element.FirstChildElement();
      while (tag)
      {
        std::string value = tag->ValueStr();
        bool skip(false);
        if (hasPosX && (value == "left" || value == "right" || value == "centerleft" || value == "centerright"))
          skip = true;
        if (hasPosY && (value == "top" || value == "bottom" || value == "centertop" || value == "centerbottom"))
          skip = true;
        // we insert at the end of block
        if (!skip)
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
      INFO::InfoPtr conditionID = g_infoManager.Register(condition);
      bool value = conditionID->Get();

      if (xmlIncludeConditions)
        (*xmlIncludeConditions)[conditionID] = value;

      if (!value)
      {
        include = include->NextSiblingElement("include");
        continue;
      }
    }
    std::string tagName = include->FirstChild()->Value();
    map<std::string, TiXmlElement>::const_iterator it = m_includes.find(tagName);
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

  // run through this element's attributes, resolving any constants
  TiXmlAttribute *attribute = node->FirstAttribute();
  while (attribute)
  { // check the attribute against our set
    if (m_constantAttributes.count(attribute->Name()))
      attribute->SetValue(ResolveConstant(attribute->ValueStr()));
    attribute = attribute->Next();
  }
  // also do the value
  if (node->FirstChild() && node->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT && m_constantNodes.count(node->ValueStr()))
    node->FirstChild()->SetValue(ResolveConstant(node->FirstChild()->ValueStr()));
}

std::string CGUIIncludes::ResolveConstant(const std::string &constant) const
{
  vector<string> values = StringUtils::Split(constant, ",");
  for (vector<string>::iterator i = values.begin(); i != values.end(); ++i)
  {
    map<std::string, std::string>::const_iterator it = m_constants.find(*i);
    if (it != m_constants.end())
      *i = it->second;
  }
  return StringUtils::Join(values, ",");
}

const INFO::CSkinVariableString* CGUIIncludes::CreateSkinVariable(const std::string& name, int context)
{
  map<std::string, TiXmlElement>::const_iterator it = m_skinvariables.find(name);
  if (it != m_skinvariables.end())
    return INFO::CSkinVariable::CreateFromXML(it->second, context);
  return NULL;
}
