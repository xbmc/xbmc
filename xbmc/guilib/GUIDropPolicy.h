#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "boost/shared_ptr.hpp"
#include "utils/Archive.h"


class CFileItemList;
class CFileItem;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

/*!
 \brief Interface responsible to decide what drag&drop capabilities a CFileItemList has.
 If you want to make the content of a Directory to be reorderable and or dropable, you can do so by setting an IGUIDropPolicy on it
 
 */
struct IGUIDropPolicy
{
  IGUIDropPolicy() : m_bReorderable(false) {}
  IGUIDropPolicy(bool reorderable) : m_bReorderable(reorderable) {}
  
  /*! \brief returns true, when the directory is reorderable, false otherwise
   */
  bool IsReorderable() const { return m_bReorderable; }
  /*! \brief returns true, when the given item can be dropped on the List
   */
  virtual bool IsDropable(const CFileItemPtr& item) const = 0;
  /*! \brief This function should return a pointer to an exact copy of this drop policy
   */
  virtual IGUIDropPolicy* Copy() const = 0;
  /*! \brief Function that should handle the case, when a user dropped an external item on the list
   Will only be called if IsDropable(item) returned true.
   \param list The list where the item was dropped on. (NOTE: this list already contains the dropped item)
   \param item The item that was dropped
   \param position The position where the item was inserted
   \result true on success
   \sa IsDropable
   \sa OnDropMove
   */
  virtual bool OnDropAdd(CFileItemList& list, CFileItemPtr item, int position) = 0;
  /*! \brief Function that should handle the case when the user reordered an item in our list
   Will only be called if IsReorderable() returned true
   \param list The list where the reordering took place. (NOTE: this list is already reordered!)
   \param posBefore The position where the item was before the dragging started
   \param posAfter The position where the item is now. 
   \result true on success
   */
  virtual bool OnDropMove(CFileItemList& list, int posBefore, int posAfter) = 0;
  virtual void operator>>(CArchive& ar) = 0;
  /*! \brief Responsible for creating the list element that will actually be inserted
   E.g. When you drag a item with the path: C:/great.mp3 to the favourites list, we need an element where path is
   set to: PlayMedia(C:/great.mp3). Those kinds of things should be handled in this function.
   Default Implementation: Exact Copy of the given item
   NOTE: you cannot just return the given item, you need to create a copy!
   \param item the item that is dragged
   \result the new item that is ready to be inserted.
   */
  virtual CFileItemPtr CreateDummy(const CFileItemPtr item);
  /*! \brief Responsible to determine if the item that is dragged, is already in our list
   NOTE: this function will get the original dragged item, not the dummy created with CreateDummy
   \param item The original item being dragged
   \param the list it is dragged on
   \result true, if it is already on the list
   */
  virtual bool IsDuplicate(const CFileItemPtr& item, const CFileItemList& list);
  
  /*! \brief Factory Function that creates a DropPolicy from an archive
   */
  static IGUIDropPolicy* Create(CArchive& ar);
protected:
  bool m_bReorderable;
};

enum DropPolicyType
{
  DPT_NONE = 0,
  DPT_MUSIC_PLAYLIST,
  DPT_VIDEO_PLAYLIST
};


struct PlaylistDropPolicy : public IGUIDropPolicy
{
  PlaylistDropPolicy() : IGUIDropPolicy(true) {}
  virtual bool OnAdd(CFileItemList& list, CFileItemPtr item, int position, int PlaylistType);
  virtual bool OnMove(CFileItemList& list, int posBefore, int posAfter, int PlaylistType);
};

struct MusicPlaylistDropPolicy : public PlaylistDropPolicy
{
  
  virtual bool IsDropable(const CFileItemPtr& item) const;
  virtual IGUIDropPolicy* Copy() const { return new MusicPlaylistDropPolicy(); }
  virtual void operator>>(CArchive& ar) { ar << DPT_MUSIC_PLAYLIST; }
  
  virtual bool OnDropAdd(CFileItemList& list, CFileItemPtr item, int position);
  virtual bool OnDropMove(CFileItemList& list, int posBefore, int posAfter);
protected:
};

struct VideoPlaylistDropPolicy : public PlaylistDropPolicy
{
  
  virtual bool IsDropable(const CFileItemPtr& item) const;
  virtual IGUIDropPolicy* Copy() const { return new VideoPlaylistDropPolicy(); }
  virtual void operator>>(CArchive& ar) { ar << DPT_VIDEO_PLAYLIST; ar << m_path; }
  
  
  virtual bool OnDropAdd(CFileItemList& list, CFileItemPtr item, int position);
  virtual bool OnDropMove(CFileItemList& list, int posBefore, int posAfter);
protected:
  CStdString m_path;
};

