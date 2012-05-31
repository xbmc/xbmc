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

#pragma once
#include "CueParserCallback.h"
#include "CStdStringHash.hpp"
#include "FileItem.h"
#include "music/Song.h"
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

class CueReader;
class CFileItemList;

/*! Callback class for collecting metadata provided by CueParser. 
 */
class CueSongsCollector
  : public CueParserCallback
{
  const CFileItemList& m_container;
  CStdString m_cueFile;
  bool m_external;
  typedef std::pair<CStdString, MUSIC_INFO::CMusicInfoTag> CacheItem;
  typedef boost::unordered_map<CStdString, MUSIC_INFO::CMusicInfoTag, CStdStringHash> Cache;
  Cache m_cache;
  boost::shared_ptr<CueReader> m_reader;
  VECSONGS m_songs;
  boost::unordered_set<CStdString, CStdStringHash> itemstodelete;
private:
  const CFileItemList& container();
  const MUSIC_INFO::CMusicInfoTag& tag(const CStdString& mediaFile);
public:
  CueSongsCollector(const CFileItemList& container);
  /*! Load CUE-data from \b filePath and detect type of cuesheet.
   * Returns true on success.
   */
  bool load(const CStdString &filePath);
  /*! Provide next string to parser.
   */
  virtual bool onDataNeeded(CStdString& dataLine);
  /*! Method called by parser when it found new file item.
   */
  virtual bool onFile(CStdString& filePath);
  /*! Collect information about each track.
   */
  virtual bool onTrackReady(CSong& song);
  /*! Finalize collecting process.
   * Method delete all CUE and related media files from items list.
   * Also appends all found media files to the list.
   */
  void finalize(VECFILEITEMS& items);
  virtual ~CueSongsCollector();
};
