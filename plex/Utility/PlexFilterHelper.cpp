//
//  PlexFilterHelper.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-05.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexFilterHelper.h"
#include "guilib/GUIControlGroupList.h"
#include "GUI/GUIDialogFilterSort.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "plex/PlexUtils.h"
#include "plex/FileSystem/PlexDirectory.h"
#include "GUIUserMessages.h"
#include "AdvancedSettings.h"
#include "guilib/GUILabelControl.h"
#include "GUI/GUIDialogFilterSort.h"
#include "GUIWindowManager.h"

using namespace XFILE;
using namespace std;

typedef pair<CStdString, CPlexFilterPtr> name_filter_pair;

bool CPlexFilterHelper::ApplySort(int ctrlId)
{
  m_appliedSort.clear();

  BOOST_FOREACH(name_filter_pair pr, m_sorts)
  {
    if (pr.second->GetControlID() == ctrlId)
    {
      m_appliedSort = pr.second->GetFilterName();
      m_sortDirectionAsc = ((CGUIRadioButtonControl*)pr.second->GetFilterControl())->IsSelected();
      return true;
    }
  }
  return false;
}

bool CPlexFilterHelper::SkinHasFilters() const
{
  CGUIControlGroupList *filterGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(FILTER_LIST);
  CGUIControlGroupList *sortGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(SORT_LIST);

  if (!filterGroup || !sortGroup)
    return false;

  return true;
}

void CPlexFilterHelper::ApplyFilterFromDialog(CPlexFilterPtr filter)
{
  if (m_appliedFilters.find(filter->GetFilterName()) != m_appliedFilters.end())
    m_appliedFilters.erase(filter->GetFilterName());

  if (!filter->GetFilterValue().empty())
    m_appliedFilters[filter->GetFilterName()] = filter->GetFilterValue();

  BOOST_FOREACH(name_filter_pair p, m_filters)
    p.second->SetAppliedFilters(m_appliedFilters);
}

bool CPlexFilterHelper::ApplyFilter(int ctrlId)
{
  bool update = false;
  BOOST_FOREACH(name_filter_pair pr, m_filters)
  {
    if (pr.second->GetControlID() == ctrlId)
    {
      if (pr.second->IsBooleanType())
      {
        /* Clear this filter first */
        if (m_appliedFilters.find(pr.second->GetFilterName()) != m_appliedFilters.end())
          m_appliedFilters.erase(pr.second->GetFilterName());

        if (!pr.second->GetFilterValue().empty())
          m_appliedFilters[pr.second->GetFilterName()] = pr.second->GetFilterValue();

        update = true;
        break;
      }
      else
      {
        CGUIDialogFilterSort *dialog = (CGUIDialogFilterSort *)g_windowManager.GetWindow(WINDOW_DIALOG_FILTER_SORT);
        if (!dialog)
          break;
        dialog->SetFilter(pr.second);
        dialog->SetFilterHelper(this);
        dialog->DoModal();
      }
    }
  }
  return update;
}

void CPlexFilterHelper::BuildFilters(const CURL& baseUrl, EPlexDirectoryType type)
{
  CGUIControlGroupList *filterGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(FILTER_LIST);
  CGUIControlGroupList *sortGroup = (CGUIControlGroupList*)m_mediaWindow->GetControl(SORT_LIST);

  if (!filterGroup || !sortGroup)
    return;

  filterGroup->ClearAll();
  sortGroup->ClearAll();

  CGUIButtonControl* originalButton = (CGUIButtonControl*)m_mediaWindow->GetControl(FILTER_BUTTON);
  if (!originalButton)
    return;
  originalButton->SetVisible(false);

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl*)m_mediaWindow->GetControl(FILTER_RADIO_BUTTON);
  if (!radioButton)
    return;
  radioButton->SetVisible(false);

  CGUIRadioButtonControl* sortButton = (CGUIRadioButtonControl*)m_mediaWindow->GetControl(SORT_RADIO_BUTTON);
  if (!sortButton)
    return;
  sortButton->SetVisible(false);

  CURL filterUrl(baseUrl);
  PlexUtils::AppendPathToURL(filterUrl, "filters");
  filterUrl.SetOption("type", boost::lexical_cast<std::string>(type));

  /* Fetch Filters */
  CFileItemList filterItems;
  CPlexDirectory dir;
  if (!dir.GetDirectory(filterUrl, filterItems))
    return;
  
  std::map<CStdString, CPlexFilterPtr> newFilters;

  for(int i = 0; i < filterItems.Size(); i++)
  {
    CFileItemPtr item = filterItems.Get(i);
    CPlexFilterPtr filter;
    CStdString filterName = item->GetProperty("filter").asString();
    if (m_filters.find(filterName) == m_filters.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item));
    else
      filter = m_filters[filterName];

    newFilters[filterName] = filter;
    filterGroup->AddControl(filter->NewFilterControl(filter->IsBooleanType() ? radioButton : originalButton, FILTER_BUTTONS_START + i));
    CLog::Log(LOGDEBUG, "CPlexFilterHelper::BuildFilters adding %s to filterGroup", filter->GetFilterName().c_str());
  }

  m_filters = newFilters;

  /* Fetch sorts */
  newFilters.clear();
  CURL sortUrl(baseUrl);
  PlexUtils::AppendPathToURL(sortUrl, "sorts");
  sortUrl.SetOption("type", boost::lexical_cast<std::string>(type));
  
  /* Fetch Filters */
  CFileItemList sortItems;
  if (!dir.GetDirectory(sortUrl, sortItems))
    return;

  for(int i = 0; i < sortItems.Size(); i++)
  {
    CFileItemPtr item = sortItems.Get(i);
    CPlexFilterPtr filter;
    CStdString sortName = item->GetProperty("unprocessed_key").asString();
    if (m_sorts.find(sortName) == m_sorts.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item));
    else
      filter = m_sorts[sortName];

    newFilters[sortName] = filter;
    sortGroup->AddControl(filter->NewFilterControl(sortButton, SORT_BUTTONS_START + i));
  }

  m_sorts = newFilters;
}

CURL CPlexFilterHelper::GetFilterUrl(const CStdString& exclude, const CURL& baseUrl) const
{
  if (m_appliedFilters.size() > 0)
  {
    CURL filterUrl(baseUrl);
    pair<CStdString, string> stpair;
    BOOST_FOREACH(stpair, m_appliedFilters)
    {
      if (!stpair.first.Equals(exclude))
        filterUrl.SetOption(stpair.first, stpair.second);
    }
    return filterUrl;
  }
  return baseUrl;
}

void CPlexFilterHelper::ClearFilters()
{
  m_appliedFilters.clear();
  m_filters.clear();
  m_sorts.clear();
}

CURL CPlexFilterHelper::GetRealDirectoryUrl(const CStdString& url_, bool& secondary)
{
  CFileItemList tmpItems;
  CPlexDirectory dir;
  CURL dirUrl(url_);

  if (!SkinHasFilters())
    return url_;

  secondary = false;

  if (m_mapToSection.Get() == dirUrl.Get())
    dirUrl = m_sectionUrl;

  CURL tempURL(dirUrl);
  tempURL.SetProtocolOption("containerSize", "1");
  tempURL.SetProtocolOption("containerStart", "0");
  
  tmpItems.SetPath(dirUrl.Get());

  /* A bit stupidity here, but we need to request the container twice. Fortunately it's really fast
   * to do it with the offset stuff above */
  if (dir.GetDirectory(tempURL, tmpItems))
  {
    if (tmpItems.IsPlexMediaServer() && tmpItems.GetContent() == "secondary")
    {
      if (m_sectionUrl.Get() != dirUrl.Get())
      {
        m_sectionUrl = dirUrl;
        ClearFilters();
      }

      CURL url(dirUrl);
      if (tmpItems.GetProperty("HomeVideoSection").asBoolean())
        PlexUtils::AppendPathToURL(url, "folder");
      else
        PlexUtils::AppendPathToURL(url, "all");

      url = GetFilterUrl("", url);

      if (!m_appliedSort.empty())
        url.SetOption("sort", m_appliedSort + ":" + (m_sortDirectionAsc ? "asc" : "desc"));

      /* Save this */
      m_mapToSection = url;

      secondary = true;
      return url;
    }
    else if (tmpItems.IsPlexMediaServer() && (tmpItems.GetContent() == "seasons" || tmpItems.GetContent() == "albums"))
    {
      if (tmpItems.GetProperty("totalSize").asInteger() == 1 && g_advancedSettings.m_bCollapseSingleSeason)
      {
        CFileItemPtr season = tmpItems.Get(0);
        CStdString url = season->GetPath();
        return url;
      }
    }
  }
  return dirUrl;
}
