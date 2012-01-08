/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifdef HAS_DS_PLAYER

#include "FilterCoreFactory.h"
#include "DSPlayer.h"
#include "filters/XBMCFileSource.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"

InternalFilters internalFilters[] =
{
  {"internal_archivesource", "Internal archive source", &InternalFilterConstructor<CXBMCASyncReader>}
};

HRESULT CFilterCoreFactory::LoadMediasConfiguration(TiXmlElement* pConfig )
{
  if (!pConfig || strcmpi(pConfig->Value(), "mediasconfig") != 0)
  {
    CLog::Log(LOGERROR, "Error loading medias configuration, no <mediasconfig> node");
    return E_FAIL;
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

  return S_OK;
}

HRESULT CFilterCoreFactory::LoadFiltersConfiguration(TiXmlElement* pConfig )
{
  if (!pConfig || strcmpi(pConfig->Value(), "filtersconfig") != 0)
  {
    CLog::Log(LOGERROR, "Error loading filters configuration, no <filtersconfig> node");
    return E_FAIL;
  }

  /* Parse filters declaration */

  TiXmlElement *pFilters = pConfig->FirstChildElement("filters");
  if (pFilters)
  {
    TiXmlElement *pFilter = pFilters->FirstChildElement("filter");
    CStdString type = "";
    while (pFilter)
    {
      m_Filters.push_back(new CFGFilterFile(pFilter));
      pFilter = pFilter->NextSiblingElement("filter");
    }
  }

  return S_OK;
}

CGlobalFilterSelectionRule* CFilterCoreFactory::GetGlobalFilterSelectionRule( const CFileItem& pFileItem, bool checkUrl /*= false*/ )
{
  for (ULONG i = 0; i < m_selecRules.size(); i++)
  {
    if (m_selecRules[i]->Match(pFileItem, checkUrl))
      return m_selecRules[i];
  }

  return NULL;
}

HRESULT CFilterCoreFactory::GetSourceFilter( const CFileItem& pFileItem, CStdString& filter )
{
  filter = "";

  /* Special case for internet stream, and rar archive */
  // TODO: handle DVD
  if (URIUtils::IsInArchive(pFileItem.GetPath()))
  {
    filter = "internal_archivesource";
    return S_OK;
  }

  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem, true);
  if (! pRule)
  {
    if (pFileItem.IsInternetStream())
    {
      filter = "internal_urlsource";
      return S_OK;
    }
    return E_FAIL;
  }

  std::vector<CStdString> foo;
  pRule->GetSourceFilters(pFileItem, foo);

  if (foo.empty())
    return E_FAIL;

  filter = foo[0];
  return S_OK;
}

HRESULT CFilterCoreFactory::GetSplitterFilter( const CFileItem& pFileItem, CStdString& filter )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  std::vector<CStdString> foo;
  pRule->GetSplitterFilters(pFileItem, foo);

  if (foo.empty())
    return E_FAIL;

  filter = foo[0];
  return S_OK;
}

HRESULT CFilterCoreFactory::GetAudioRendererFilter( const CFileItem& pFileItem, CStdString& filter )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  std::vector<CStdString> foo;
  pRule->GetAudioRendererFilters(pFileItem, foo);

  if (foo.empty())
    return E_FAIL;

  filter = foo[0];
  return S_OK;
}

HRESULT CFilterCoreFactory::GetAudioFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/ )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  std::vector<CStdString> foo;
  pRule->GetAudioFilters(pFileItem, foo, dxva);

  if (foo.empty())
    return E_FAIL;

  filter = foo[0];
  return S_OK;;
}

HRESULT CFilterCoreFactory::GetVideoFilter( const CFileItem& pFileItem, CStdString& filter, bool dxva /*= false*/ )
{
  filter = "";
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  std::vector<CStdString> foo;
  pRule->GetVideoFilters(pFileItem, foo, dxva);

  if (foo.empty())
    return E_FAIL; //Todo: Error message

  filter = foo[0];
  return S_OK;
}

HRESULT CFilterCoreFactory::GetExtraFilters( const CFileItem& pFileItem, std::vector<CStdString>& filters, bool dxva /*= false*/ )
{
  filters.clear();
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  pRule->GetExtraFilters(pFileItem, filters, dxva);
  return S_OK;
}

HRESULT CFilterCoreFactory::GetShaders(const CFileItem& pFileItem, std::vector<uint32_t>& shaders, bool dxva /*= false*/)
{
  shaders.clear();
  CGlobalFilterSelectionRule * pRule = GetGlobalFilterSelectionRule(pFileItem);
  if (! pRule)
    return E_FAIL;

  pRule->GetShaders(pFileItem, shaders, dxva);
  return S_OK;
}

CFGFilter* CFilterCoreFactory::GetFilterFromName( const CStdString& _filter, bool showError )
{
  CStdString filter = _filter;
  
  // Right now we only have the rar source filter
  for (int i = 0; i < countof(internalFilters); i++)
  {
    if (internalFilters[i].name.Equals(filter))
    {
      return internalFilters[i].cst(internalFilters[i].osdname);
    }
  }

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
        CDSPlayer::errorWindow = dialog;
      }
    }

    return NULL;

  }

  return (*it);
}
std::vector<CGlobalFilterSelectionRule *> CFilterCoreFactory::m_selecRules;
std::vector<CFGFilterFile *> CFilterCoreFactory::m_Filters;

#endif