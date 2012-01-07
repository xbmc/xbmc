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

#include "recplayer.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

cRecPlayer::cRecPlayer(cRecording* rec)
{
  m_file          = -1;
  m_fileOpen      = -1;
  m_recordingFilename = strdup(rec->FileName());

  // FIXME find out max file path / name lengths
#if VDRVERSNUM < 10703
  m_pesrecording = true;
  m_indexFile = new cIndexFile(m_recordingFilename, false);
#else
  m_pesrecording = rec->IsPesRecording();
  if(m_pesrecording) INFOLOG("recording '%s' is a PES recording", m_recordingFilename);
  m_indexFile = new cIndexFile(m_recordingFilename, false, m_pesrecording);
#endif

  scan();
}

void cRecPlayer::cleanup() {
  for(int i = 0; i != m_segments.Size(); i++) {
    delete m_segments[i];
  }
  m_segments.Clear();
}

void cRecPlayer::scan()
{
  struct stat s;

  closeFile();

  m_totalLength = 0;
  m_fileOpen    = -1;
  m_totalFrames = 0;

  cleanup();

  for(int i = 0; ; i++) // i think we only need one possible loop
  {
    fileNameFromIndex(i);

    if(stat(m_fileName, &s) == -1) {
      break;
    }

    cSegment* segment = new cSegment();
    segment->start = m_totalLength;
    segment->end = segment->start + s.st_size;

    m_segments.Append(segment);

    m_totalLength += s.st_size;
    INFOLOG("File %i found, size: %llu, totalLength now %llu", i, s.st_size, m_totalLength);
  }

  m_totalFrames = m_indexFile->Last();
  INFOLOG("total frames: %u", m_totalFrames);
}

cRecPlayer::~cRecPlayer()
{
  cleanup();
  closeFile();
  free(m_recordingFilename);
}

char* cRecPlayer::fileNameFromIndex(int index) {
  if (m_pesrecording)
    snprintf(m_fileName, sizeof(m_fileName), "%s/%03i.vdr", m_recordingFilename, index+1);
  else
    snprintf(m_fileName, sizeof(m_fileName), "%s/%05i.ts", m_recordingFilename, index+1);

  return m_fileName;
}

bool cRecPlayer::openFile(int index)
{
  if (index == m_fileOpen) return true;
  closeFile();

  fileNameFromIndex(index);
  INFOLOG("openFile called for index %i string:%s", index, m_fileName);

  m_file = open(m_fileName, O_RDONLY | O_NOATIME);
  if (m_file == -1)
  {
    INFOLOG("file failed to open");
    m_fileOpen = -1;
    return false;
  }
  m_fileOpen = index;
  return true;
}

void cRecPlayer::closeFile()
{
  if(m_file == -1) {
    return;
  }

  INFOLOG("file closed");
  close(m_file);

  m_file = -1;
  m_fileOpen = -1;
}

uint64_t cRecPlayer::getLengthBytes()
{
  return m_totalLength;
}

uint32_t cRecPlayer::getLengthFrames()
{
  return m_totalFrames;
}

int cRecPlayer::getBlock(unsigned char* buffer, uint64_t position, int amount)
{
  // dont let the block be larger than 256 kb
  if (amount > 256*1024)
    amount = 256*1024;

  if ((uint64_t)amount > m_totalLength)
    amount = m_totalLength;

  if (position >= m_totalLength)
    return 0;

  if ((position + amount) > m_totalLength)
    amount = m_totalLength - position;

  // work out what block "position" is in
  int segmentNumber = -1;
  for(int i = 0; i < m_segments.Size(); i++)
  {
    if ((position >= m_segments[i]->start) && (position < m_segments[i]->end)) {
      segmentNumber = i;
      break;
    }
  }

  // segment not found / invalid position
  if (segmentNumber == -1) return 0;

  // open file (if not already open)
  if (!openFile(segmentNumber)) return 0;

  // work out position in current file
  uint64_t filePosition = position - m_segments[segmentNumber]->start;

  // seek to position
  if(lseek(m_file, filePosition, SEEK_SET) == -1) {
    ERRORLOG("unable to seek to position: %llu", filePosition);
    return 0;
  }

  // try to read the block
  int bytes_read = read(m_file, buffer, amount);
  INFOLOG("read %i bytes from file %i at position %llu", bytes_read, segmentNumber, filePosition);

  if(bytes_read <= 0) {
    return 0;
  }

  // Tell linux not to bother keeping the data in the FS cache
  posix_fadvise(m_file, filePosition, bytes_read, POSIX_FADV_DONTNEED);

  // divide and conquer
  if(bytes_read < amount) {
    bytes_read += getBlock(&buffer[bytes_read], position + bytes_read, amount - bytes_read);
  }

  return bytes_read;
}

uint64_t cRecPlayer::positionFromFrameNumber(uint32_t frameNumber)
{
  if (!m_indexFile) return 0;
#if VDRVERSNUM < 10703
  unsigned char retFileNumber;
  int retFileOffset;
  unsigned char retPicType;
#else
  uint16_t retFileNumber;
  off_t retFileOffset;
  bool retPicType;
#endif
  int retLength;


  if (!m_indexFile->Get((int)frameNumber, &retFileNumber, &retFileOffset, &retPicType, &retLength))
    return 0;

  if (retFileNumber >= m_segments.Size()) 
    return 0;

  uint64_t position = m_segments[retFileNumber]->start + retFileOffset;
  return position;
}

uint32_t cRecPlayer::frameNumberFromPosition(uint64_t position)
{
  if (!m_indexFile) return 0;

  if (position >= m_totalLength)
  {
    DEBUGLOG("Client asked for data starting past end of recording!");
    return m_totalFrames;
  }

  int segmentNumber = -1;
  for(int i = 0; i < m_segments.Size(); i++)
  {
    if ((position >= m_segments[i]->start) && (position < m_segments[i]->end)) {
      segmentNumber = i;
      break;
    }
  }

  if(segmentNumber == -1) {
    return m_totalFrames;
  }

  uint32_t askposition = position - m_segments[segmentNumber]->start;
  return m_indexFile->Get((int)segmentNumber, askposition);
}


bool cRecPlayer::getNextIFrame(uint32_t frameNumber, uint32_t direction, uint64_t* rfilePosition, uint32_t* rframeNumber, uint32_t* rframeLength)
{
  // 0 = backwards
  // 1 = forwards

  if (!m_indexFile) return false;

#if VDRVERSNUM < 10703
  unsigned char waste1;
  int waste2;
#else
  uint16_t waste1;
  off_t waste2;
#endif

  int iframeLength;
  int indexReturnFrameNumber;

  indexReturnFrameNumber = (uint32_t)m_indexFile->GetNextIFrame(frameNumber, (direction==1 ? true : false), &waste1, &waste2, &iframeLength);
  DEBUGLOG("GNIF input framenumber:%u, direction=%u, output:framenumber=%i, framelength=%i", frameNumber, direction, indexReturnFrameNumber, iframeLength);

  if (indexReturnFrameNumber == -1) return false;

  *rfilePosition = positionFromFrameNumber(indexReturnFrameNumber);
  *rframeNumber = (uint32_t)indexReturnFrameNumber;
  *rframeLength = (uint32_t)iframeLength;

  return true;
}
