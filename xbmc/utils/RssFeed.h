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

#ifndef RSS_FEED_H_
#define RSS_FEED_H_

#include <string>
#include <vector>

#include <time.h>

#include "FileItem.h"

/**
 * The purpose of this class is to describe an RSS feed
 */
class CRssFeed 
{
public:
  CRssFeed();
  virtual ~CRssFeed();

  bool Init(const CStdString& strURL);
  void GetItemList(CFileItemList &feedItems) 
  {
    for (int i = 0; i < items.Size(); i++) {
      feedItems.Add(items[i]);
    }
  }

  const CStdString& GetUrl() { return m_strURL; }
  const CStdString& GetFeedTitle() { return m_strTitle; }
  
  bool ReadFeed();

private:
  time_t ParseDate(const CStdString & strDate);
  
  bool IsPathToMedia(const CStdString& strPath );
  bool IsPathToThumbnail(const CStdString& strPath );

  CFileItemList items;
  
  // Lock that protects the feed vector
  CCriticalSection m_ItemVectorLock;

  CStdString m_strURL;
  
  // Channel information
  CStdString m_strTitle;
  CStdString m_strAuthor;
  CStdString m_strLink;
  CStdString m_strGuid; // globally unique identifier of the item in the entire feed
  CStdString m_strDescription;
  CStdString m_strThumbnail;
};


#endif /*RSS_FEED_H_*/
