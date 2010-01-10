#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "DVDDemux.h"
#include <map>

class CDVDDemuxPVRClient;

class CDemuxStreamVideoPVRClient : public CDemuxStreamVideo
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamVideoPVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDemuxStreamAudioPVRClient : public CDemuxStreamAudio
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamAudioPVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDemuxStreamSubtitlePVRClient : public CDemuxStreamSubtitle
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamSubtitlePVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};


class CDVDDemuxPVRClient : public CDVDDemux
{
public:

  CDVDDemuxPVRClient();
  ~CDVDDemuxPVRClient();

  /*
   * Open input stream
   */
  bool Open(CDVDInputStream* pInput);

  /*
   * Reset the entire demuxer (same result as closing and opening it)
   */
  void Reset();

  /*
   * Aborts any internal reading that might be stalling main thread
   * NOTICE - this can be called from another thread
   */
  void Abort();

  /*
   * Flush the demuxer, if any data is kept in buffers, this should be freed now
   */
  void Flush();

  /*
   * Read a packet, returns NULL on error
   *
   */
  DemuxPacket* Read();

  /*
   * Seek, time in msec calculated from stream start
   */
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL) { return false; }

  /*
   * Seek to a specified chapter.
   * startpts can be updated to the point where display should start
   */
  bool SeekChapter(int chapter, double* startpts = NULL) { return false; }

  /*
   * Get the number of chapters available
   */
  int GetChapterCount() { return 0; }

  /*
   * Get current chapter
   */
  int GetChapter() { return -1; }

  /*
   * Get the name of the current chapter
   */
  void GetChapterName(std::string& strChapterName) {}

  /*
   * Set the playspeed, if demuxer can handle different
   * speeds of playback
   */
  void SetSpeed(int iSpeed) {};

  /*
   * returns the total time in msec
   */
  int GetStreamLength();

  /*
   * returns the stream or NULL on error, starting from 0
   */
  CDemuxStream* GetStream(int iStreamId);

  /*
   * return nr of streams, 0 if none
   */
  int GetNrOfStreams();

  /*
   * returns opened filename
   */
  std::string GetFileName();

  /*
   * return a user-presentable codec name of the given stream
   */
  void GetStreamCodecName(int iStreamId, CStdString &strName) {};

protected:
  CRITICAL_SECTION m_critSection;

  CDVDInputStream* m_pInput;

  std::map<int, CDemuxStream*> m_streams;

private:
  /*
   * Request streams from PVRClient
   */
  void RequestStreams();
};

