/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "GUIWindowMusicTop100.h"
#include "Util.h"
#include "Application.h"
#include "PlayListPlayer.h"


#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4

#define CONTROL_LABELFILES        12

#define CONTROL_LIST       50
#define CONTROL_BIGLIST      53

CGUIWindowMusicTop100::CGUIWindowMusicTop100(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_TOP100, "MyMusicTop100.xml")
{
}

CGUIWindowMusicTop100::~CGUIWindowMusicTop100(void)
{
}

bool CGUIWindowMusicTop100::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_iViewAsIconsRoot = g_stSettings.m_iMyMusicTop100ViewAsIcons;

      CGUIWindowMusicBase::OnMessage(message);

      return true;
    }
    break;

  case GUI_MSG_DVDDRIVE_EJECTED_CD:
  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    return true;
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        m_iViewAsIconsRoot++;
        if (m_iViewAsIconsRoot > VIEW_AS_LARGE_LIST) m_iViewAsIconsRoot = VIEW_AS_LIST;
        if (m_iViewAsIconsRoot != VIEW_AS_LIST) m_iViewAsIconsRoot = VIEW_AS_LARGE_LIST;
        g_stSettings.m_iMyMusicTop100ViewAsIcons = m_iViewAsIconsRoot;
        g_settings.Save();
        UpdateButtons();
        return true;
      }
    }
    break;
  }

  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicTop100::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    // Top 100 has no parent dirs
    return true;
  }

  return CGUIWindowMusicBase::OnAction(action);
}

bool CGUIWindowMusicTop100::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size() )
  {
    // cleanup items;
    items.Clear();
  }

  VECSONGS songs;
  g_musicDatabase.GetTop100(songs);

  // Display an error message if the database doesn't contain any top 100 songs
  DisplayEmptyDatabaseMessage(songs.empty());

  for (int i = 0; i < (int)songs.size(); ++i)
  {
    CSong & song = songs[i];
    CFileItem* pFileItem = new CFileItem(song.strTitle);
    pFileItem->m_strPath = song.strFileName;
    pFileItem->m_bIsFolder = false;
    pFileItem->m_musicInfoTag.SetAlbum(song.strAlbum);
    pFileItem->m_musicInfoTag.SetArtist(song.strArtist);
    pFileItem->m_musicInfoTag.SetGenre(song.strGenre);
    pFileItem->m_musicInfoTag.SetDuration(song.iDuration);
    pFileItem->m_musicInfoTag.SetTitle(song.strTitle);
    pFileItem->m_musicInfoTag.SetTrackNumber(song.iTrack);
    pFileItem->m_musicInfoTag.SetLoaded(true);
    items.Add(pFileItem);
  }

  return true;
}

void CGUIWindowMusicTop100::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  CONTROL_DISABLE(CONTROL_BTNSORTBY);
  CONTROL_DISABLE(CONTROL_BTNSORTASC);

  m_viewControl.SetCurrentView(m_iViewAsIconsRoot);

  // Update object count label
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);
}

void CGUIWindowMusicTop100::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];

  // done a search?
  if (pItem->GetLabel() == "..")
  {
    Update("");
    return ;
  }

  g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
  g_playlistPlayer.Reset();

  for ( int i = 0; i < (int) m_vecItems.Size(); i++ )
  {
    CFileItem* pItem = m_vecItems[i];

    CPlayList::CPlayListItem playlistItem;
    CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
    g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
  }

  // Save current window and directroy to know where the selected item was
  m_nTempPlayListWindow = GetID();
  m_strTempPlayListDirectory = m_Directory.m_strPath;
  CUtil::RemoveSlashAtEnd(m_strTempPlayListDirectory);

  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
  g_playlistPlayer.Play(iItem);
}

void CGUIWindowMusicTop100::OnFileItemFormatLabel(CFileItem* pItem)
{
  if (pItem->m_musicInfoTag.Loaded())
  { // set label 1
    SetLabelFromTag(pItem);
  }

  // set thumbs and default icons
  pItem->SetMusicThumb();
  pItem->FillInDefaultIcon();

  // CONTROL_THUMB in top 100 is a listcontrol, so big
  // iconimages have to be shown if its enabled
//  if (ViewByIcon())
  pItem->SetIconImage(pItem->GetThumbnailImage());
}

void CGUIWindowMusicTop100::DoSort(CFileItemList& items)
{
}

void CGUIWindowMusicTop100::OnSearchItemFound(const CFileItem* pSelItem)
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == pSelItem->m_strPath)
    {
      m_viewControl.SetSelectedItem(i);
      m_viewControl.SetFocused();
    }
  }
}

/// \brief Search for a song or a artist with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicTop100::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    CMusicInfoTag& tag = pItem->m_musicInfoTag;

    CStdString strArtist = tag.GetArtist();
    strArtist.MakeLower();
    CStdString strTitle = tag.GetTitle();
    strTitle.MakeLower();
    CStdString strAlbum = tag.GetAlbum();
    strAlbum.MakeLower();

    if (strArtist.Find(strSearch) > -1 || strTitle.Find(strSearch) > -1 || strAlbum.Find(strSearch) > -1 )
    {
      CStdString strSong = g_localizeStrings.Get(179); // Song
      CFileItem* pNewItem = new CFileItem(*pItem);
      pNewItem->SetLabel("[" + strSong + "] " + tag.GetTitle() + " - " + tag.GetArtist());
      items.Add(pNewItem);
    }
  }
}