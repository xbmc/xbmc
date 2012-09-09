/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "ModuleXbmcplugin.h"

#include "filesystem/PluginDirectory.h"
#include "FileItem.h"

namespace XBMCAddon
{

  namespace xbmcplugin
  {
    bool addDirectoryItem(int handle, const String& url, const xbmcgui::ListItem* listItem,
                          bool isFolder, int totalItems)
    {
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
      for (std::vector<Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool> >::const_iterator item = items.begin();
           item < items.end(); item++ )
      {
        const Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool>* pItem = &(*item);
        const String& url = pItem->first();
        const XBMCAddon::xbmcgui::ListItem *pListItem = pItem->second();
        bool bIsFolder = pItem->GetNumValuesSet() > 2 ? pItem->third() : false;
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
      AddonClass::Ref<xbmcgui::ListItem> pListItem(listItem);
      XFILE::CPluginDirectory::SetResolvedUrl(handle, succeeded, pListItem->item.get());
    }

    void addSortMethod(int handle, int sortMethod, const String& clabel2Mask)
    {
      String label2Mask;
      label2Mask = (clabel2Mask.empty() ? "%D" : clabel2Mask.c_str());

      // call the directory class to add the sort method.
      if (sortMethod >= SORT_METHOD_NONE && sortMethod < SORT_METHOD_MAX)
        XFILE::CPluginDirectory::AddSortMethod(handle, (SORT_METHOD)sortMethod, label2Mask);
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
