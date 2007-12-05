#include "include.h"
#include "GUIColorManager.h"
#include "../xbmc/Util.h"
#include "SkinInfo.h"

CGUIColorManager g_colorManager;

CGUIColorManager::CGUIColorManager(void)
{
}

CGUIColorManager::~CGUIColorManager(void)
{
  Clear();
}

void CGUIColorManager::Clear()
{
  m_colors.clear();
}

// load the color file in
void CGUIColorManager::Load(const CStdString &colorFile)
{
  Clear();

  // first load the default color map if it exists
  CStdString path, basePath;
  CUtil::AddFileToFolder(g_SkinInfo.GetBaseDir(), "colors", basePath);
  CUtil::AddFileToFolder(basePath, "defaults.xml", path);

  TiXmlDocument xmlDoc;
  if (xmlDoc.LoadFile(path.c_str()))
    LoadXML(xmlDoc);

  // now the color map requested
  if (colorFile.CompareNoCase("SKINDEFAULT") == 0)
    return; // nothing to do

  CUtil::AddFileToFolder(basePath, colorFile, path);
  CLog::Log(LOGINFO, "Loading colors from %s", path.c_str());

  if (xmlDoc.LoadFile(path.c_str()))
    LoadXML(xmlDoc);
}

bool CGUIColorManager::LoadXML(TiXmlDocument &xmlDoc)
{
  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("colors"))
  {
    CLog::Log(LOGERROR, "color file doesnt start with <colors>");
    return false;
  }

  const TiXmlElement *color = pRootElement->FirstChildElement("color");

  while (color)
  {
    if (color->FirstChild() && color->Attribute("name"))
    {
      DWORD value = 0xffffffff;
      sscanf(color->FirstChild()->Value(), "%x", &value);
      CStdString name = color->Attribute("name");
      iColor it = m_colors.find(name);
      if (it != m_colors.end())
        (*it).second = value;
      else
        m_colors.insert(make_pair(name, value));
    }
    color = color->NextSiblingElement("color");
  }
  return true;
}

// lookup a color and return it's hex value
DWORD CGUIColorManager::GetColor(const CStdString &color) const
{
  // look in our color map
  CStdString trimmed(color);
  trimmed.TrimLeft("= ");
  icColor it = m_colors.find(trimmed);
  if (it != m_colors.end())
    return (*it).second;

  // try converting hex directly
  DWORD value = 0;
  sscanf(trimmed.c_str(), "%lx", &value);
  return value;
}
