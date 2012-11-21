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

  CGUISpinControlEx* spinControl = (CGUISpinControlEx *)GetControl(FILTER_SPIN_CONTROL);
  spinControl->SetVisible(false);


  /* Fetch Filters */
  CFileItemList filterItems;
  FetchFilterSortList(baseUrl, "filters", type, filterItems);
  int startId = -100;

  for(int i = 0; i < filterItems.Size(); i++)
  {
    CFileItemPtr item = filterItems.Get(i);
    if (!item)
      break;

    CStdString filterType = item->GetProperty("filterType").asString();
    if (filterType == "string")
    {
      CGUIButtonControl* filterButton = new CGUIButtonControl(*originalButton);
      filterButton->SetLabel(item->GetLabel());
      filterButton->SetVisible(true);
      filterButton->AllocResources();
      filterButton->SetID(startId + i);
      filterGroup->AddControl(filterButton);
    }
    else if (filterType == "boolean")
    {
      CGUIRadioButtonControl* filterRButton = new CGUIRadioButtonControl(*radioButton);
      filterRButton->SetLabel(item->GetLabel());
      filterRButton->SetVisible(true);
      filterRButton->AllocResources();
      filterRButton->SetID(startId + i);
      filterGroup->AddControl(filterRButton);
    }
    else if (filterType == "integer")
    {
      CGUISpinControlEx* filterSpin = new CGUISpinControlEx(*spinControl);
      //filterSpin->SetLabel(item->GetLabel());
      filterSpin->SetVisible(true);
      filterSpin->AllocResources();
      filterSpin->SetID(startId + i);
      filterGroup->AddControl(filterSpin);
    }
  }

  /* Fetch sorts */
  CFileItemList sortItems;
  FetchFilterSortList(baseUrl, "sorts", type, sortItems);
  startId = -200;

  for(int i = 0; i < sortItems.Size(); i++)
  {
    CFileItemPtr item = sortItems.Get(i);
    if (!item)
      break;

    CGUIButtonControl* sortButton = new CGUIButtonControl(*originalButton);
    sortButton->SetLabel(item->GetLabel());
    sortButton->SetVisible(true);
    sortButton->AllocResources();
    sortButton->SetID(startId + i);

    sortGroup->AddControl(sortButton);
  }

}

bool CGUIWindowMediaFilterView::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
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
      CLog::Log(LOGDEBUG, "Found a PlexMediaServer Filter directory, going to default to load /all");

      CStdString url;
      url = PlexUtils::AppendPathToURL(strDirectory, "all");

      /* Since we are fucking around with which directory that will be loaded
       * we need to reset these variables. Not pretty, but working */
      m_history.ClearPathHistory();
      m_startDirectory = url;

      ret = CGUIWindowVideoNav::GetDirectory(url, items);
      if (ret)
      {
        /* Now load the filters */
        int type = 1;
        if (items.HasProperty("typeNumber"))
          type = items.GetProperty("typeNumber").asInteger();

        BuildFilters(strDirectory, type);
      }

      return ret;
    }
  }

  return CGUIWindowVideoNav::GetDirectory(strDirectory, items);
}
