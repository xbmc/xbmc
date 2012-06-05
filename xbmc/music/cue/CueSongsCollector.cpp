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

#include "CueSongsCollector.h"
#include "TagCueReader.h"
#include "FileCueReader.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "settings/Settings.h"


CueSongsCollector::CueSongsCollector(const CFileItemList& container)
  : m_container(container)
  , m_external(true)
{
}

const CFileItemList& CueSongsCollector::container()
{
  return m_container;
}

const MUSIC_INFO::CMusicInfoTag& CueSongsCollector::tag(const CStdString& mediaFile)
{
  // We cache MusicTag for prevent extra reading from disk.
  Cache::iterator it = m_cache.find(mediaFile);
  if (it == m_cache.end())
    it = m_cache.insert(CacheItem(mediaFile, MUSIC_INFO::CMusicInfoTag(mediaFile))).first;
  return it->second;
}

bool CueSongsCollector::load(const CStdString &filePath)
{
  m_cueFile = filePath;
  CStdString extension;
  URIUtils::GetExtension(m_cueFile, extension);
  if (extension.Equals((char*)".cue"))
  {
    m_reader = boost::shared_ptr<CueReader>(new FileCueReader(m_cueFile));
    m_external = true;
  }
  else
  {
    if (extension.Equals((char*)".ape") || extension.Equals((char*)".flac") ||
      extension.Equals((char*)".wv"))
    {
      const MUSIC_INFO::CMusicInfoTag& t = tag(filePath);
      if (t.HasEmbeddedCue())
      {
        m_reader = boost::shared_ptr<CueReader>(new TagCueReader(t.GetEmbeddedCue()));
        m_external = false;
      }
    }
  }
  bool result = false;
  if (m_reader)
    result = m_reader->isValid();
  if (result)
    itemstodelete.insert(m_cueFile);
  return result;
}

bool CueSongsCollector::onFile(CStdString& filePath)
{
  bool bFoundMediaFile = true;
  if (m_external)
  {
    // Convert relative path to absolute.
    if ((filePath.Left(1) != "/") && filePath.Mid(1, 3) != ":\\\\")
      filePath = URIUtils::AddFileToFolder(container().GetPath(), filePath);
    CStdString mediaFilePath = filePath;
    bFoundMediaFile = XFILE::CFile::Exists(filePath) || container().Contains(filePath);
    if (!bFoundMediaFile)
    {
      // try removing the .cue extension...
      URIUtils::RemoveExtension(filePath);
      CFileItem item(filePath, false);
      bFoundMediaFile = (item.IsAudio() && container().Contains(filePath));
    }
    if (!bFoundMediaFile)
    {
      // try replacing the extension with one of our allowed ones.
      CStdStringArray extensions;
      StringUtils::SplitString(g_settings.m_musicExtensions, "|", extensions);
      for (unsigned int i = 0; i < extensions.size(); i++)
      {
        filePath = URIUtils::ReplaceExtension(mediaFilePath, extensions[i]);
        CFileItem item(filePath, false);
        if (!item.IsCUESheet() && !item.IsPlayList() && container().Contains(filePath))
        {
          bFoundMediaFile = true;
          break;
        }
      }
    }
    if (bFoundMediaFile)
    { // if file has a cue, we stop parsing.
      if (tag(filePath).HasEmbeddedCue())
      {
        itemstodelete.insert(filePath);
        return false;
      }
    }
  }
  else // For embedded we always use self file
    filePath = m_cueFile;
  return bFoundMediaFile;
}

bool CueSongsCollector::onDataNeeded(CStdString& dataLine)
{
  return m_reader->ReadNextLine(dataLine);
}

bool CueSongsCollector::onTrackReady(CSong& song)
{
  // we might have a new media file from parser
  const MUSIC_INFO::CMusicInfoTag& t = tag(song.strFileName);
  if (t.Loaded())
  {
    if (song.strAlbum.empty() && !t.GetAlbum().empty())
      song.strAlbum = t.GetAlbum();
    if (song.albumArtist.empty() && !t.GetAlbumArtist().empty())
      song.albumArtist = t.GetAlbumArtist();
    if (song.genre.empty() && !t.GetGenre().empty())
      song.genre = t.GetGenre();
    if (song.artist.empty() && !t.GetArtist().empty())
      song.artist = t.GetArtist();
    if (t.GetDiscNumber())
      song.iTrack |= (t.GetDiscNumber() << 16); // see CMusicInfoTag::GetDiscNumber()
    SYSTEMTIME dateTime;
    t.GetReleaseDate(dateTime);
    if (dateTime.wYear)
      song.iYear = dateTime.wYear;
  }
  if (!song.iDuration && t.GetDuration() > 0)
  { // must be the last song
    song.iDuration = (t.GetDuration() * 75 - song.iStartOffset + 37) / 75;
  }
  // add this item to the list
  m_songs.push_back(song);
  return true;
}

void CueSongsCollector::finalize(VECFILEITEMS& items)
{
  // now delete the .CUE files and underlying media files.
  for (boost::unordered_set<CStdString>::const_iterator removeIt = itemstodelete.begin();
    removeIt != itemstodelete.end(); ++removeIt)
  {
    for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
    {
      CFileItemPtr pItem = *it;
      if (stricmp(pItem->GetPath().c_str(), removeIt->c_str()) == 0)
      {
        items.erase(it);
        break;
      }
    }
  }
  for (VECSONGS::const_iterator it = m_songs.begin(); it != m_songs.end(); ++it)
  {
    CFileItemPtr pItem(new CFileItem(*it));
    items.push_back(pItem);
  }
}

CueSongsCollector::~CueSongsCollector()
{
}
