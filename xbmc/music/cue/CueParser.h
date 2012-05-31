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
#include "CueSheetIndexList.h"
#include "music/Song.h"
#include "CueMetaData.h"

class CueParserCallback;

/*! This class handles .cue format. The .cue-sheet contains all the
 * track and timing information.  An example file is:
 *
 * PERFORMER "Pink Floyd"
 * TITLE "The Dark Side Of The Moon"
 * PERFORMER "Pink Floyd"
 * FILE "The Dark Side Of The Moon.mp3" WAVE
 *   TRACK 01 AUDIO
 *     TITLE "Speak To Me / Breathe"
 *     INDEX 00 00:00:00
 *     INDEX 01 00:00:32
 *   TRACK 02 AUDIO
 *     TITLE "On The Run"
 *     INDEX 00 03:58:72
 *     INDEX 01 04:00:72
 *   TRACK 03 AUDIO
 *     TITLE "Time"
 *     INDEX 00 07:31:70
 *     INDEX 01 07:33:70
 */
class CueParser
{
private:
  struct GlobalInfo {
    CueMetaData m_meta;
    CStdString m_flags;
    void reset();
  };
  struct TrackInfo {
    CStdString m_file;
    CueMetaData m_meta; // All data (artist, title, year...)
    CueSheetIndexList m_indexes;
    unsigned m_endOffset;
    unsigned m_trackNumber;
    unsigned m_pregap;
    void reset();
  };
  
  typedef std::vector<TrackInfo> Tracks;

  GlobalInfo m_globalData; // Global info for all tracks
  Tracks m_tracks;

  // Used by parser... Temporary...
  CStdString m_file;
  TrackInfo m_currentTrack; // Current parsed track
  bool m_is_va;
  CStdString m_albumArtist;
public:
  CueParser();
  void reset();
  /*! Run parsing and set callback for parser.
   */
  bool parse(CueParserCallback& callback);
  struct TrackEntry
  {
    CStdString m_file;
    unsigned m_trackNumber;
    CueSheetIndexList m_indexes;
  };
private:
  bool onMeta(const CStdString& name, const CStdString& value);
  bool onIndex(const CStdString& strLine);
  bool onTrack(const CStdString& strLine);
  bool onFile(const CStdString& strLine);
  bool onFlags(const CStdString& strLine);
  bool onPregap(const CStdString& strLine);

  bool finalizeTrack();

  // Helper methods
  static bool extractTime(const CStdString& strLine, unsigned& time);
  static bool extractString(const CStdString& line, CStdString& outStr, bool spaceIsError);
  static CStdString cutFirstWord(CStdString& inputStr);
};
