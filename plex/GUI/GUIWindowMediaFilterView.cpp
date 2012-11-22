//
//  GUIWindowMediaFilterView.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIWindowMediaFilterView.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "plex/PlexUtils.h"
#include "plex/FileSystem/PlexDirectory.h"

bool CGUIWindowMediaFilterView::FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list)
{
  CStdString filterUrl;
  CStdString typeStr;
  typeStr.Format("?type=%d", type);

  filterUrl = PlexUtils::AppendPathToURL(url, filterSort);
  filterUrl = PlexUtils::AppendPathToURL(filterUrl, typeStr);

  CPlexDirectory fdir(true, false);
  return fdir.GetDirectory(filterUrl, list);
}

void CGUIWindowMediaFilterView::BuildFilters(const CStdString& baseUrl, int type)
{
  CGUIControlGroupList *filterGroup = (CGUIControlGroupList*)GetControl(FILTER_LIST);
  CGUIControlGroupList *sortGroup = (CGUIControlGroupList*)GetControl(SORT_LIST);

  if (!filterGroup || !sortGroup)
    return;

  filterGroup->ClearAll();
  sortGroup->ClearAll();

  CGUIButtonControl* originalButton = (CGUIButtonControl *)GetControl(FILTER_BUTTON);
  originalButton->SetVisible(false);

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl *)GetControl(FILTER_RADIO_BUTTON);
  radioButton->SetVisible(false);

  /* Fetch Filters */
  CFileItemList filterItems;
  FetchFilterSortList(baseUrl, "filters", type, filterItems);
  std::map<CStdString, CPlexFilterPtr> newFilters;

  for(int i = 0; i < filterItems.Size(); i++)
  {
    CFileItemPtr item = filterItems.Get(i);
    CPlexFilterPtr filter;
    CStdString filterName = item->GetProperty("filter").asString();
    if (m_filters.find(filterName) == m_filters.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item->GetLabel(), filterName, item->GetProperty("filterType").asString(), item->GetProperty("key").asString()));
    else
      filter = m_filters[filterName];

    newFilters[filterName] = filter;
    filterGroup->AddControl(filter->NewFilterControl(filter->IsBooleanType() ? radioButton : originalButton, FILTER_BUTTONS_START + i));
  }

  m_filters = newFilters;

  /* Fetch sorts */
  newFilters.clear();
  CFileItemList sortItems;
  FetchFilterSortList(baseUrl, "sorts", type, sortItems);

  for(int i = 0; i < sortItems.Size(); i++)
  {
    CFileItemPtr item = filterItems.Get(i);
    CPlexFilterPtr filter;
    CStdString sortName = item->GetProperty("key").asString();
    if (m_sorts.find(sortName) == m_sorts.end())
      /* No such filter yet */
      filter = CPlexFilterPtr(new CPlexFilter(item->GetLabel(), sortName, item->GetProperty("filterType").asString(), item->GetProperty("key").asString()));
    else
      filter = m_sorts[sortName];

    newFilters[sortName] = filter;
    sortGroup->AddControl(filter->NewFilterControl(filter->IsBooleanType() ? radioButton : originalButton, SORT_BUTTONS_START + i));
  }

  m_sorts = newFilters;

}

typedef pair<CStdString, CPlexFilterPtr> name_filter_pair;

bool CGUIWindowMediaFilterView::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIWindowVideoNav::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      dprintf("Clicked with CtrlID = %d", ctrlId);
      if (ctrlId < 0 && ctrlId >= -100)
      {
        BOOST_FOREACH(name_filter_pair pr, m_filters)
        {
          if (pr.second->GetControlID() == ctrlId)
          {
            dprintf("Clicked filter %s", pr.second->GetFilterName().c_str());
            if (!pr.second->GetFilterValue().empty())
            {
              m_appliedFilters.push_back(pr.second->GetFilterValue());
              update = true;
            }
          }
        }

        if (update)
          Update(m_baseUrl, true);
      }
      else
      {
      }
    }
  }

  return ret;
}

bool CGUIWindowMediaFilterView::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  bool ret;
  CFileItemList tmpItems;
  CPlexDirectory dir(true, true);
  tmpItems.SetPath(strDirectory);

  /* Ok this is kind of wastefull */
  if (dir.GetDirectory(strDirectory, tmpItems))
  {
    if (tmpItems.IsPlexMediaServer() && tmpItems.GetContent() == "secondary")
    {
      CStdString url;
      url = PlexUtils::AppendPathToURL(strDirectory, "all");

      if (m_appliedFilters.size() > 0)
      {
        CStdString optionList = StringUtils::Join(m_appliedFilters, "&");
        url += "?" + optionList;
      }

      m_baseUrl = strDirectory;

      ret = CGUIWindowVideoNav::Update(url, updateFilterPath);

      /* Kill the history */
      m_history.ClearPathHistory();
      m_startDirectory = url;

      if (ret)
      {
        int type = 1;
        if (m_vecItems->HasProperty("typeNumber"))
          type = m_vecItems->GetProperty("typeNumber").asInteger();
        BuildFilters(strDirectory, type);
      }

      return ret;
    }
  }

  return CGUIWindowVideoNav::Update(strDirectory, updateFilterPath);
}
