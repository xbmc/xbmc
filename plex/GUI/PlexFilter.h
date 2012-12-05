//
//  PlexFilter.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-26.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXFILTER_H
#define PLEXFILTER_H

#include "StdString.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "FileSystem/PlexDirectory.h"

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

class CPlexFilter
{
  public:
    CPlexFilter(const CStdString& filterString, const CStdString& filterName, const CStdString& filterType, const CStdString& key)
      : m_filterName(filterName)
      , m_filterType(filterType)
      , m_filterString(filterString)
      , m_key(key)
    {
      m_filterControl = NULL;
    }

    CGUIButtonControl* NewFilterControl(CGUIButtonControl *parent, int id)
    {
      if (IsBooleanType() || m_filterType.empty())
        m_filterControl = new CGUIRadioButtonControl(*(CGUIRadioButtonControl*)parent);
      else
      {
        m_filterControl = new CGUIButtonControl(*parent);
      }

      m_filterControl->SetLabel(m_filterString);
      m_filterControl->AllocResources();
      m_filterControl->SetID(id);
      m_filterControl->SetVisible(true);

      return m_filterControl;
    }

    CGUIButtonControl* GetFilterControl() const { return m_filterControl; }

    bool IsStringType() const { return m_filterType == "string"; }
    bool IsIntegerType() const { return m_filterType == "integer"; }
    bool IsBooleanType() const { return m_filterType == "boolean"; }
    bool IsActive() const { return !m_currentValue.empty(); }

    CStdString GetFilterName() const { return m_filterName; }
    CStdString GetFilterString() const { return m_filterString; }

    int GetControlID() const
    {
      if (m_filterControl)
        return m_filterControl->GetID();

      return 0;
    }

    CStdString GetFilterValue() const
    {
      CStdString filterStr;
      if (IsBooleanType())
      {
        CGUIRadioButtonControl* radio = (CGUIRadioButtonControl*)m_filterControl;
        if (radio->IsSelected())
          filterStr.Format("%s=1", m_filterName);
      }
      else
      {
        if (m_currentValue.empty())
          return "";
        CStdString values = StringUtils::Join(m_currentValue, ",");
        filterStr.Format("%s=%s", m_filterName, values);
      }

      return filterStr;
    }

    bool GetSublist(CFileItemList& sublist)
    {
      /* FIXME: needs to be made ASync? */
      if (m_sublist.Size() == 0)
        if (!FetchSublist())
          return false;

      sublist.Copy(m_sublist);
      return true;
    }

    void UpdateLabel()
    {
      CStdString newLabel(m_filterString);
      if (m_currentValue.size() > 0)
      {
        newLabel = "[B]" + m_filterString;

        CStdString primaryKey = m_currentValue[0];
        CFileItemList list;
        if (GetSublist(list))
        {
          for (int i = 0; i < list.Size(); i ++)
          {
            CFileItemPtr item = list.Get(i);
            if (item->GetProperty("unprocessedKey").asString() == primaryKey)
            {
              newLabel += ": " + item->GetLabel();
              break;
            }
          }
          if (m_currentValue.size() > 1)
          {
            CStdString plusStr;
            plusStr.Format("(+%d)", m_currentValue.size() - 1);
            newLabel += " " + plusStr;
          }
        }
        newLabel += "[/B]";
      }
      m_filterControl->SetLabel(newLabel);
    }

    void AddCurrentValue(const CStdString& value)
    {
      m_currentValue.push_back(value);
      UpdateLabel();
    }

    void ClearCurrentValue()
    {
      m_currentValue.clear();
      UpdateLabel();
    }

    void RemoveCurrentValue(const CStdString& value)
    {
      m_currentValue.erase(std::remove(m_currentValue.begin(), m_currentValue.end(), value), m_currentValue.end());
      UpdateLabel();
    }

    void SetFilterUrl(const CStdString& filterUrl)
    {
      m_filterUrl = filterUrl;
      m_sublist.Clear();
    }

    bool HasCurrentValue(const CStdString& valueKey)
    {
      bool found = false;
      BOOST_FOREACH(std::string& value, m_currentValue)
      {
        if (value.compare(valueKey) == 0)
        {
          found = true;
          break;
        }
      }
      return found;
    }

  private:
    bool FetchSublist()
    {
      CPlexDirectory dir;
      CStdString url(m_key);

      if (!m_filterUrl.empty())
      {
        if (url.Find('?') == -1)
          url += m_filterUrl;
        else
          url += "&" + m_filterUrl.substr(1);
      }

      return dir.GetDirectory(url, m_sublist);
    }

    CGUIButtonControl* m_filterControl;
    CStdString m_filterName;
    CStdString m_filterString;
    CStdString m_filterType;
    std::vector<std::string> m_currentValue;
    CStdString m_key;
    CStdString m_filterUrl;

    CFileItemList m_sublist;
};

typedef boost::shared_ptr<CPlexFilter> CPlexFilterPtr;


#endif // PLEXFILTER_H
