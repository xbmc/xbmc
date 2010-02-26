#include "FilterCoreFactory.h"

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
  filter = "";
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
  filter = "";
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

bool CFilterCoreFactory::GetAudioRendererFilter( const CFileItem& pFileItem, CStdString& filter, SStreamInfos* s )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetAudioRendererFilters(pFileItem, foo, s);

  if (foo.empty())
    return false;

  filter = foo[0];
  return true;
}

bool CFilterCoreFactory::GetAudioFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/, SStreamInfos* s )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  std::vector<CStdString> foo;
  pRule->GetAudioFilters(pFileItem, foo, dxva, s);

  if (foo.empty())
    return false;

  filter = foo[0];
  return true;;
}

bool CFilterCoreFactory::GetVideoFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/ )
{
  filter = "";
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
  filters.clear();
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return false;

  pRule->GetExtraFilters(pFileItem, filters, dxva);
  return true;
}

CFGFilterFile* CFilterCoreFactory::GetFilterFromName( const CStdString& filter, bool showError )
{
  std::vector<CFGFilterFile *>::const_iterator it = std::find_if(m_Filters.begin(),
    m_Filters.end(), std::bind2nd(std::ptr_fun(CompareCFGFilterFileToString), filter) );

  if (it == m_Filters.end())
  {
    CLog::Log(LOGERROR, "%s Filter \"%s\" isn't loaded. Please check dsfilterconfig.xml", __FUNCTION__, filter.c_str());
    if (showError)
    {
      CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dialog)
      {
        dialog->SetHeading("Filter not found");
        dialog->SetLine(0, "The filter \"" + filter + "\" isn't loaded.");
        dialog->SetLine(1, "Please check your dsfilterconfig.xml.");
        dialog->DoModal();
      }
    }

    return NULL;

  }

  return (*it);
}
std::vector<CGlobalFilterSelectionRule *> CFilterCoreFactory::m_selecRules;
std::vector<CFGFilterFile *> CFilterCoreFactory::m_Filters;
