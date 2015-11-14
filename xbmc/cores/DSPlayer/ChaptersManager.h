/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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

#ifndef HAS_DS_PLAYER
#pragma error "DSPlayer's header file included without HAS_DS_PLAYER defined"
#endif

#include "DSUtil/SmartPtr.h"

// IAMExtendedSeeking
#include <qnetwork.h>

#include "DSGraph.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

/// \brief Contains informations about a chapter
struct SChapterInfos
{
  CStdString name; ///< Chapter's name
  uint64_t starttime; ///< Chapter's start time (in ms)
  uint64_t endtime; ///< Chapter's end time (in ms; not used)
};

/** DSPlayer Chapters Manager.

  Singleton class handling chapters management
  */
class CChaptersManager
{
public:
  /// Retrieve singleton instance
  static CChaptersManager *Get();
  /// Destroy the singleton instance
  static void Destroy();

  /** Retrieve the chapters count.
  @return Number of chapters in the media file
  */
  int GetChapterCount(void);
  /** Retrive the current chapter.
   * @return ID of the current chapter
   */
  int GetChapter(void);

  int GetChapterPos(int iChapter);

  /** Retrive current chapter's name
   * @param[out] strChapterName The chapter's name
   */
  void GetChapterName(std::string& strChapterName, int chapterIdx);
  /** Sync the current chapter with the media file */
  void UpdateChapters(int64_t current_time);
  /** Seek to the specified chapter
   * @param iChapter Chapter to seek to
   * @return Always 0
   */
  int SeekChapter(int iChapter);
  /** Load the chapters from the media file
   * @return True if succeeded, false otherwise
   */
  bool LoadInterface(void);
  /** Load the IAMExtendedSeeking interface
   * @return True if succeeded, false otherwise
   */
  bool LoadChapters(void);

  /** Does the media file have chapters?
   * @return True if the media file has chapters, false otherwise
   */
  bool HasChapters() const
  {
    return m_chapters.size() > 0;
  }

  void FlushChapters(void);
private:
  /// Constructor
  CChaptersManager(void);
  /// Destructor
  ~CChaptersManager(void);
  /// Store singleton instance
  static CChaptersManager *m_pSingleton;

  /// Store SChapterInfos structs
  std::map<long, SChapterInfos *> m_chapters;
  /// ID of the current chapter
  long m_currentChapter;
  /// Pointer to the IAMExtendedSeeking interface
  Com::SmartQIPtr<IAMExtendedSeeking, &IID_IAMExtendedSeeking> m_pIAMExtendedSeeking;

  CCriticalSection m_lock;
};