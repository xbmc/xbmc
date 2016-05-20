/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_PlayList.h"

#include "Application.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "addons/binary/AddonInterfaceManager.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "utils/URIUtils.h"

using namespace ADDON;
using namespace PLAYLIST;

namespace V2
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

void CAddOnPlayList::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AddonPlayList.New                          = V2::KodiAPI::Player::CAddOnPlayList::New;
  interfaces->AddonPlayList.Delete                       = V2::KodiAPI::Player::CAddOnPlayList::Delete;
  interfaces->AddonPlayList.LoadPlaylist                 = V2::KodiAPI::Player::CAddOnPlayList::LoadPlaylist;
  interfaces->AddonPlayList.AddItemURL                   = V2::KodiAPI::Player::CAddOnPlayList::AddItemURL;
  interfaces->AddonPlayList.AddItemList                  = V2::KodiAPI::Player::CAddOnPlayList::AddItemList;
  interfaces->AddonPlayList.RemoveItem                   = V2::KodiAPI::Player::CAddOnPlayList::RemoveItem;
  interfaces->AddonPlayList.ClearList                    = V2::KodiAPI::Player::CAddOnPlayList::ClearList;
  interfaces->AddonPlayList.GetListSize                  = V2::KodiAPI::Player::CAddOnPlayList::GetListSize;
  interfaces->AddonPlayList.GetListPosition              = V2::KodiAPI::Player::CAddOnPlayList::GetListPosition;
  interfaces->AddonPlayList.Shuffle                      = V2::KodiAPI::Player::CAddOnPlayList::Shuffle;
}

void* CAddOnPlayList::New(void *hdl, int playList)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p')",
                                        __FUNCTION__, helper);

      // we do not create our own playlist, just using the ones from playlistplayer
      if (playList != PLAYLIST_MUSIC && playList != PLAYLIST_VIDEO && playList != PLAYLIST_PICTURE)
        throw ADDON::WrongValueException("CAddOnPlayList - %s: %s/%s - PlayList does not exist",
                                           __FUNCTION__,
                                           TranslateType(helper->GetAddon()->Type()).c_str(),
                                           helper->GetAddon()->Name().c_str());

    return &g_playlistPlayer.GetPlaylist(playList);;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnPlayList::Delete(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);
    // Nothing to do, present if needed in future (to prevent API change)
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayList::LoadPlaylist(void* hdl, void* handle, const char* filename, int playList)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle || !filename)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p', filename='%p')",
                                        __FUNCTION__, helper, handle, filename);

    CFileItem item(filename);
    item.SetPath(filename);

    if (item.IsPlayList())
    {
      // load playlist and copy all items to existing playlist

      // load a playlist like .m3u, .pls
      // first get correct factory to load playlist
      std::unique_ptr<CPlayList> pPlayList(CPlayListFactory::Create(item));
      if (pPlayList.get() != nullptr)
      {
        // load it
        if (!pPlayList->Load(item.GetPath()))
        {
          //hmmm unable to load playlist?
          return false;
        }

        // clear current playlist
        g_playlistPlayer.ClearPlaylist(playList);

        // add each item of the playlist to the playlistplayer
        for (int i = 0; i < (int)pPlayList->size(); ++i)
        {
          CFileItemPtr playListItem =(*pPlayList)[i];
          if (playListItem->GetLabel().empty())
            playListItem->SetLabel(URIUtils::GetFileName(playListItem->GetPath()));

          static_cast<PLAYLIST::CPlayList*>(handle)->Add(playListItem);
        }
      }
      return true;
    }
    else
    {
      // filename is not a valid playlist
      throw ADDON::WrongValueException("CAddOnPlayList - %s: %s/%s - Not a valid playlist",
                                         __FUNCTION__,
                                         TranslateType(helper->GetAddon()->Type()).c_str(),
                                         helper->GetAddon()->Name().c_str());
    }
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayList::AddItemURL(void* hdl, void* handle, const char* url, int index)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle || !url)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p', url='%p')",
                                        __FUNCTION__, helper, handle, url);

    CFileItemPtr item(new CFileItem(url, false));
    item->SetLabel(url);

    CFileItemList items;
    items.Add(item);
    static_cast<PLAYLIST::CPlayList*>(handle)->Insert(items, index);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayList::AddItemList(void* hdl, void* handle, const void* listitem, int index)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle || !listitem)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p', listitem='%p')",
                                        __FUNCTION__, helper, handle, listitem);

    CFileItemList items;
    items.Add(std::make_shared<CFileItem>(*static_cast<const CFileItem*>(listitem)));
    static_cast<PLAYLIST::CPlayList*>(handle)->Insert(items, index);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayList::CAddOnPlayList::RemoveItem(void* hdl, void* handle, const char* url)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle || !url)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p', url='%p')",
                                        __FUNCTION__, helper, handle, url);

    static_cast<PLAYLIST::CPlayList*>(handle)->Remove(url);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayList::ClearList(void* hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<PLAYLIST::CPlayList*>(handle)->Clear();
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnPlayList::GetListSize(void* hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<PLAYLIST::CPlayList*>(handle)->size();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

int CAddOnPlayList::GetListPosition(void* hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return g_playlistPlayer.GetCurrentSong();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

void CAddOnPlayList::Shuffle(void* hdl, void* handle, bool shuffle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayList - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    if (shuffle)
      static_cast<PLAYLIST::CPlayList*>(handle)->Shuffle();
    else
      static_cast<PLAYLIST::CPlayList*>(handle)->UnShuffle();
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V2 */
