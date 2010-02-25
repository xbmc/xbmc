#pragma once

#include "FilterSelectionRule.h"
#include "tinyXML\tinyxml.h"
#include "globalfilterselectionrule.h"

class CFilterCoreFactory
{
public:
  static std::vector<CFGFilterFile *> m_Filters;

  static bool LoadConfiguration(TiXmlElement* pConfig, bool clear);
  static bool GetSourceFilter(const CFileItem& pFileItem, CStdString& filter);
  static bool GetSplitterFilter(const CFileItem& pFileItem, CStdString& filter);
  static bool GetVideoFilter(const CFileItem& pFileItem, CStdString& filter, bool dxva = false);
  static bool GetAudioFilter(const CFileItem& pFileItem, CStdString& filter, bool dxva = false);
  static bool GetExtraFilters(const CFileItem& pFileItem, std::vector<CStdString>& filters, bool dxva = false);

  static bool SomethingMatch( const CFileItem& pFileItem)
  {
    return (GetGlobalFilterSelectionRule(pFileItem) != NULL);
  }

  static void Destroy()
  {
    while(!m_Filters.empty())
    {
      if (m_Filters.back())
        delete m_Filters.back();
      m_Filters.pop_back();
    }

    while(!m_selecRules.empty())
    {
      if (m_selecRules.back())
        delete m_selecRules.back();
      m_selecRules.pop_back();
    }

    CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
  }

private:
  static CGlobalFilterSelectionRule* GetGlobalFilterSelectionRule(const CFileItem& pFileItem);

  static std::vector<CGlobalFilterSelectionRule *> m_selecRules;
};

bool CFilterCoreFactory::LoadConfiguration(TiXmlElement* pConfig, bool clear )
{
  if (clear)
  {
    Destroy();
  }

  if (!pConfig || strcmpi(pConfig->Value(), "dsfilterconfig") != 0)
  {
    CLog::Log(LOGERROR, "Error loading configuration, no <dsfilterconfig> node");
    return false;
  }

  /* Parse filters declaration */

  TiXmlElement *pFilters = pConfig->FirstChildElement("filters");
  if (pFilters)
  {
    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    while (pFilter)
    {
      m_Filters.push_back(new CFGFilterFile(pFilter));
      pFilter = pFilter->NextSiblingElement("filter");
    }
  }

  /* Parse selection rules */
  TiXmlElement *pRules = pConfig->FirstChildElement("rules");
  if (pRules)
  {
    TiXmlElement *pRule = pRules->FirstChildElement("rule");
    while (pRule)
    {
      m_selecRules.push_back(new CGlobalFilterSelectionRule(pRule));
      pRule = pRule->NextSiblingElement("rule");
    }
  }

  return true;
}

CGlobalFilterSelectionRule* CFilterCoreFactory::GetGlobalFilterSelectionRule( const CFileItem& pFileItem )
{
  for (int i = 0; i < m_selecRules.size(); i++)
  {
    if (m_selecRules[i]->Match(pFileItem))
      return m_selecRules[i];
  }

  return NULL;
}

bool CFilterCoreFactory::GetSourceFilter( const CFileItem& pFileItem, CStdString& filter )
{
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetSourceFilters(pFileItem, foo);

  if (foo.empty())
    return false;

  filter = foo[0];
  return true;
}

bool CFilterCoreFactory::GetSplitterFilter( const CFileItem& pFileItem, CStdString& filter )
{
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetSplitterFilters(pFileItem, foo);

  if (foo.empty())
    return false;

  filter = foo[0];
  return true;
}

bool CFilterCoreFactory::GetAudioFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/ )
{
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetAudioFilters(pFileItem, foo, dxva);

  if (foo.empty())
    return false;

  filter = foo[0];
  return true;;
}

bool CFilterCoreFactory::GetVideoFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/ )
{
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetVideoFilters(pFileItem, foo, dxva);

  if (foo.empty())
    return false; //Todo: Error message

  filter = foo[0];
  return true;
}

bool CFilterCoreFactory::GetExtraFilters( const CFileItem& pFileItem, std::vector<CStdString>& filters, bool dxva /*= false*/ )
{
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  pRule->GetExtraFilters(pFileItem, filters, dxva);
  return true;
}
std::vector<CGlobalFilterSelectionRule *> CFilterCoreFactory::m_selecRules;
std::vector<CFGFilterFile *> CFilterCoreFactory::m_Filters;
