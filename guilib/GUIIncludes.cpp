#include "GUIIncludes.h"

CGUIIncludes::CGUIIncludes()
{
}

CGUIIncludes::~CGUIIncludes()
{
}

bool CGUIIncludes::LoadIncludes(const CStdString &includeFile)
{
  // make sure our include map is cleared.
  m_includes.clear();

  TiXmlDocument doc;
  if (!doc.LoadFile(includeFile.c_str()))
  {
    CLog::Log(LOGINFO, "Error loading includes.xml file (%s): %s (row=%i, col=%i)", includeFile.c_str(), doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
    return false;
  }
  // success, load the tags
  // Format is:

  // <includes>
  //   <include name="tagName">
  //      tags to include
  //   </include>
  //   ...
  // </includes>
  TiXmlElement* root = doc.RootElement();
  if (!root || strcmpi(root->Value(), "includes"))
  {
    CLog::Log(LOGERROR, "Skin includes must start with the <includes> tag");
    return false;
  }
  TiXmlElement* node = root->FirstChildElement("include");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      CStdString tagName = node->Attribute("name");
      m_includes.insert(pair<CStdString, TiXmlElement>(tagName, *node));
    }
    node = node->NextSiblingElement("include");
  }
  return true;
}

void CGUIIncludes::ResolveIncludes(TiXmlElement *node)
{
  // we have a node, find any <include>tagName</include> tags and replace
  // recursively with their real includes
  if (!node) return;
  TiXmlElement *include = node->FirstChildElement("include");
  while (include && include->FirstChild())
  {
    // have an include tag - grab it's tag name and replace it with the real tag contents
    CStdString tagName = include->FirstChild()->Value();
    map<CStdString, TiXmlElement>::iterator it = m_includes.find(tagName);
    if (it != m_includes.end())
    { // found the tag(s) to include - let's replace it
      const TiXmlElement &element = (*it).second;
      TiXmlElement *tag = element.FirstChildElement();
      while (tag)
      {
        node->InsertAfterChild(include, *tag);
        tag = tag->NextSiblingElement();
      }
      // remove the <include>tagName</include> element
      node->RemoveChild(include);
    }
    else
    { // invalid include
      CLog::Log(LOGWARNING, "Skin has invalid include: %s", tagName.c_str());
      return;
    }
    include = node->FirstChildElement("include");
  }
}