/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ModuleXbmcplugin.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/PluginDirectory.h"

namespace XBMCAddon
{

  namespace xbmcplugin
  {
    bool addDirectoryItem(int handle, const String& url, const xbmcgui::ListItem* listItem,
                          bool isFolder, int totalItems)
    {
      if (listItem == nullptr)
        throw new XBMCAddon::WrongTypeException("None not allowed as argument for listitem");
      AddonClass::Ref<xbmcgui::ListItem> pListItem(listItem);
      pListItem->item->SetPath(url);
      pListItem->item->m_bIsFolder = isFolder;

      // call the directory class to add our item
      return XFILE::CPluginDirectory::AddItem(handle, pListItem->item.get(), totalItems);
    }

    bool addDirectoryItems(int handle,
                           const std::vector<Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool> >& items,
                           int totalItems)
    {
      CFileItemList fitems;
      for (const auto& item : items)
      {
        const String& url = item.first();
        const XBMCAddon::xbmcgui::ListItem* pListItem = item.second();
        bool bIsFolder = item.GetNumValuesSet() > 2 ? item.third() : false;
        pListItem->item->SetPath(url);
        pListItem->item->m_bIsFolder = bIsFolder;
        fitems.Add(pListItem->item);
      }

      // call the directory class to add our items
      return XFILE::CPluginDirectory::AddItems(handle, &fitems, totalItems);
    }

    void endOfDirectory(int handle, bool succeeded, bool updateListing,
                        bool cacheToDisc)
    {
      // tell the directory class that we're done
      XFILE::CPluginDirectory::EndOfDirectory(handle, succeeded, updateListing, cacheToDisc);
    }

    void setResolvedUrl(int handle, bool succeeded, const xbmcgui::ListItem* listItem)
    {
      if (listItem == nullptr)
        throw new XBMCAddon::WrongTypeException("None not allowed as argument for listitem");
      AddonClass::Ref<xbmcgui::ListItem> pListItem(listItem);
      XFILE::CPluginDirectory::SetResolvedUrl(handle, succeeded, pListItem->item.get());
    }

    void addSortMethod(int handle, int sortMethod, const String& clabelMask, const String& clabel2Mask)
    {
      String labelMask;
      if (sortMethod == SORT_METHOD_TRACKNUM)
        labelMask = (clabelMask.empty() ? "[%N. ]%T" : clabelMask.c_str());
      else if (sortMethod == SORT_METHOD_EPISODE || sortMethod == SORT_METHOD_PRODUCTIONCODE)
        labelMask = (clabelMask.empty() ? "%H. %T" : clabelMask.c_str());
      else
        labelMask = (clabelMask.empty() ? "%T" : clabelMask.c_str());

      String label2Mask;
      label2Mask = (clabel2Mask.empty() ? "%D" : clabel2Mask.c_str());

      // call the directory class to add the sort method.
      if (sortMethod >= SORT_METHOD_NONE && sortMethod < SORT_METHOD_MAX)
        XFILE::CPluginDirectory::AddSortMethod(handle, (SORT_METHOD)sortMethod, labelMask, label2Mask);
    }

    String getSetting(int handle, const char* id)
    {
      return XFILE::CPluginDirectory::GetSetting(handle, id);
    }

    void setSetting(int handle, const String& id, const String& value)
    {
      XFILE::CPluginDirectory::SetSetting(handle, id, value);
    }

    void setContent(int handle, const char* content)
    {
      XFILE::CPluginDirectory::SetContent(handle, content);
    }

    void setPluginCategory(int handle, const String& category)
    {
      XFILE::CPluginDirectory::SetProperty(handle, "plugincategory", category);
    }

    void setPluginFanart(int handle, const char* image,
                         const char* color1,
                         const char* color2,
                         const char* color3)
    {
      if (image)
        XFILE::CPluginDirectory::SetProperty(handle, "fanart_image", image);
      if (color1)
        XFILE::CPluginDirectory::SetProperty(handle, "fanart_color1", color1);
      if (color2)
        XFILE::CPluginDirectory::SetProperty(handle, "fanart_color2", color2);
      if (color3)
        XFILE::CPluginDirectory::SetProperty(handle, "fanart_color3", color3);
    }

    void setProperty(int handle, const char* key, const String& value)
    {
      XFILE::CPluginDirectory::SetProperty(handle, key, value);
    }

  }
}
