/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2004-2005 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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

/*
 * This code is taken from VOMP for VDR plugin.
 */

#ifndef VNSI_RECPLAYER_H
#define VNSI_RECPLAYER_H

#include <stdio.h>
#include <vdr/recording.h>
#include <vdr/tools.h>

#include "config.h"

class cSegment
{
  public:
    uint64_t start;
    uint64_t end;
};

class cRecPlayer
{
public:
  cRecPlayer(cRecording* rec);
  ~cRecPlayer();
  uint64_t getLengthBytes();
  uint32_t getLengthFrames();
  int getBlock(unsigned char* buffer, uint64_t position, int amount);

  bool openFile(int index);
  void closeFile();

  void scan();
  uint64_t positionFromFrameNumber(uint32_t frameNumber);
  uint32_t frameNumberFromPosition(uint64_t position);
  bool getNextIFrame(uint32_t frameNumber, uint32_t direction, uint64_t* rfilePosition, uint32_t* rframeNumber, uint32_t* rframeLength);

private:
  void cleanup();
  char* fileNameFromIndex(int index);
  void checkBufferSize(int s);

  char        m_fileName[512];
  cIndexFile *m_indexFile;
  int         m_file;
  int         m_fileOpen;
  cVector<cSegment*> m_segments;
  uint64_t    m_totalLength;
  uint32_t    m_totalFrames;
  char       *m_recordingFilename;
  bool        m_pesrecording;
};

#endif // VNSI_RECPLAYER_H
