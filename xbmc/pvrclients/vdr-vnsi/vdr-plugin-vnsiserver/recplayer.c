/*
 *      Copyright (C) 2004-2005 Chris Tallon
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

cRecPlayer::cRecPlayer(cRecording* rec)
{
  m_file          = NULL;
  m_fileOpen      = -1;
  m_lastPosition  = 0;
  m_recordingFilename = strdup(rec->FileName());

  // FIXME find out max file path / name lengths
#if VDRVERSNUM < 10703
  m_pesrecording = true;
  m_indexFile = new cIndexFile(m_recordingFilename, false);
#else
  m_pesrecording = rec->IsPesRecording();
  if(m_pesrecording) esyslog("recording '%s' is a PES recording", m_recordingFilename);
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
  if (m_file) fclose(m_file);
  m_totalLength = 0;
  m_fileOpen    = -1;
  m_totalFrames = 0;

  cleanup();

  for(int i = 0; i < 65535; i++) // i think we only need one possible loop
  {
    fileNameFromIndex(i);
    isyslog("FILENAME: %s", m_fileName);
    m_file = fopen(m_fileName, "r");
    if (!m_file) break;

    cSegment* s = new cSegment();
    s->start = m_totalLength;
    fseek(m_file, 0, SEEK_END);
    m_totalLength += ftell(m_file);
    m_totalFrames = m_indexFile->Last();
    s->end = m_totalLength;
    m_segments.Append(s);
    isyslog("File %i found, totalLength now %llu, numFrames = %u", i, m_totalLength, m_totalFrames);
    fclose(m_file);
  }

  m_file = NULL;
}

cRecPlayer::~cRecPlayer()
{
  cleanup();
  if (m_file) fclose(m_file);
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
  if (m_file) fclose(m_file);

  fileNameFromIndex(index);
  isyslog("openFile called for index %i string:%s", index, m_fileName);

  m_file = fopen(m_fileName, "r");
  if (!m_file)
  {
    isyslog("file failed to open");
    m_fileOpen = -1;
    return false;
  }
  m_fileOpen = index;
  return true;
}

uint64_t cRecPlayer::getLengthBytes()
{
  return m_totalLength;
}

uint32_t cRecPlayer::getLengthFrames()
{
  return m_totalFrames;
}

unsigned long cRecPlayer::getBlock(unsigned char* buffer, uint64_t position, unsigned long amount)
{
  if ((amount > m_totalLength) || (amount > 500000))
  {
    LOGCONSOLE("Amount %lu requested and rejected", amount);
    return 0;
  }

  if (position >= m_totalLength)
  {
    LOGCONSOLE("Client asked for data starting past end of recording!");
    return 0;
  }

  if ((position + amount) > m_totalLength)
  {
    LOGCONSOLE("Client asked for some data past the end of recording, adjusting amount");
    amount = m_totalLength - position;
  }

  // work out what block position is in
  int segmentNumber;
  for(segmentNumber = 0; segmentNumber < m_segments.Size(); segmentNumber++)
  {
    if ((position >= m_segments[segmentNumber]->start) && (position < m_segments[segmentNumber]->end)) break;
    // position is in this block
  }

  // we could be seeking around
  if (segmentNumber != m_fileOpen)
  {
    if (!openFile(segmentNumber)) return 0;
  }

  uint64_t currentPosition = position;
  uint32_t yetToGet = amount;
  uint32_t got = 0;
  uint32_t getFromThisSegment = 0;
  uint32_t filePosition;

  while(got < amount)
  {
    if (got)
    {
      // if(got) then we have already got some and we are back around
      // advance the file pointer to the next file
      if (!openFile(++segmentNumber)) return 0;
    }

    // is the request completely in this block?
    if ((currentPosition + yetToGet) <= m_segments[segmentNumber]->end)
      getFromThisSegment = yetToGet;
    else
      getFromThisSegment = m_segments[segmentNumber]->end - currentPosition;

    filePosition = currentPosition - m_segments[segmentNumber]->start;
    fseek(m_file, filePosition, SEEK_SET);
    fread(&buffer[got], getFromThisSegment, 1, m_file);

    // Tell linux not to bother keeping the data in the FS cache
    posix_fadvise(m_file->_fileno, filePosition, getFromThisSegment, POSIX_FADV_DONTNEED);

    got += getFromThisSegment;
    currentPosition += getFromThisSegment;
    yetToGet -= getFromThisSegment;
  }

  m_lastPosition = position;
  return got;
}

uint64_t cRecPlayer::getLastPosition()
{
  return m_lastPosition;
}

/*cRecording* cRecPlayer::getCurrentRecording()
{
  return NULL;
}*/

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
  {
    return 0;
  }

//  LOGCONSOLE("FN: %u FO: %i", retFileNumber, retFileOffset);
  if (retFileNumber >= m_segments.Size()) return 0;
//  if (!m_segments[retFileNumber]) return 0;
  uint64_t position = m_segments[retFileNumber]->start + retFileOffset;
//  LOGCONSOLE("Pos: %llu", position);

  return position;
}

uint32_t cRecPlayer::frameNumberFromPosition(uint64_t position)
{
  if (!m_indexFile) return 0;

  if (position >= m_totalLength)
  {
    LOGCONSOLE("Client asked for data starting past end of recording!");
    return 0;
  }

  unsigned char segmentNumber;
  for(segmentNumber = 0; segmentNumber < m_segments.Size(); segmentNumber++)
  {
    if ((position >= m_segments[segmentNumber]->start) && (position < m_segments[segmentNumber]->end)) break;
    // position is in this block
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
  LOGCONSOLE("GNIF input framenumber:%lu, direction=%lu, output:framenumber=%i, framelength=%i", frameNumber, direction, indexReturnFrameNumber, iframeLength);

  if (indexReturnFrameNumber == -1) return false;

  *rfilePosition = positionFromFrameNumber(indexReturnFrameNumber);
  *rframeNumber = (uint32_t)indexReturnFrameNumber;
  *rframeLength = (uint32_t)iframeLength;

  return true;
}
