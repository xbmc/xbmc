/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDDemuxVobsub.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDClock.h"
#include "DVDSubtitles/DVDSubtitleStream.h"

#include <string.h>

using namespace std;

CDVDDemuxVobsub::CDVDDemuxVobsub()
{
}

CDVDDemuxVobsub::~CDVDDemuxVobsub()
{
  for(unsigned i=0;i<m_Streams.size();i++)
  {
    delete m_Streams[i];
  }
  m_Streams.clear();
}

bool CDVDDemuxVobsub::Open(const string& filename, const string& subfilename)
{
  m_Filename = filename;

  unique_ptr<CDVDSubtitleStream> pStream(new CDVDSubtitleStream());
  if(!pStream->Open(filename))
    return false;

  string vobsub = subfilename;
  if ( vobsub == "")
  {
    vobsub = filename;
    vobsub.erase(vobsub.rfind('.'), vobsub.size());
    vobsub += ".sub";
  }

  m_Input.reset(CDVDFactoryInputStream::CreateInputStream(NULL, vobsub, ""));
  if(!m_Input.get() || !m_Input->Open(vobsub.c_str(), "video/x-vobsub"))
    return false;

  m_Demuxer.reset(new CDVDDemuxFFmpeg());
  if(!m_Demuxer->Open(m_Input.get()))
    return false;

  CDVDStreamInfo hints;
  CDVDCodecOptions options;
  hints.codec = AV_CODEC_ID_DVD_SUBTITLE;

  char line[2048];
  DECLARE_UNUSED(bool,res)

  SState state;
  state.delay = 0;
  state.id = -1;

  while( pStream->ReadLine(line, sizeof(line)) )
  {
    if (*line == 0 || *line == '\r' || *line == '\n' || *line == '#')
      continue;
    else if (strncmp("langidx:", line, 8) == 0)
      res = ParseLangIdx(state, line + 8);
    else if (strncmp("delay:", line, 6) == 0)
      res = ParseDelay(state, line + 6);
    else if (strncmp("id:", line, 3) == 0)
      res = ParseId(state, line + 3);
    else if (strncmp("timestamp:", line, 10) == 0)
      res = ParseTimestamp(state, line + 10);
    else if (strncmp("palette:", line, 8) == 0
         ||  strncmp("size:", line, 5) == 0
         ||  strncmp("org:", line, 4) == 0
         ||  strncmp("custom colors:", line, 14) == 0
         ||  strncmp("scale:", line, 6) == 0
         ||  strncmp("alpha:", line, 6) == 0
         ||  strncmp("fadein/out:", line, 11) == 0
         ||  strncmp("forced subs:", line, 12) == 0)
      res = ParseExtra(state, line);
    else
      continue;
  }

  struct sorter s;
  sort(m_Timestamps.begin(), m_Timestamps.end(), s);
  m_Timestamp = m_Timestamps.begin();

  for(unsigned i=0;i<m_Streams.size();i++)
  {
    m_Streams[i]->ExtraSize = state.extra.length()+1;
    m_Streams[i]->ExtraData = new uint8_t[m_Streams[i]->ExtraSize];
    strcpy((char*)m_Streams[i]->ExtraData, state.extra.c_str());
  }

  return true;
}

void CDVDDemuxVobsub::Reset()
{
  Flush();
}

void CDVDDemuxVobsub::Flush()
{
  m_Demuxer->Flush();
}

bool CDVDDemuxVobsub::SeekTime(int time, bool backwords, double* startpts)
{
  double pts = DVD_MSEC_TO_TIME(time);
  m_Timestamp = m_Timestamps.begin();
  for(;m_Timestamp != m_Timestamps.end();++m_Timestamp)
  {
    if(m_Timestamp->pts > pts)
      break;
  }
  for(unsigned i=0;i<m_Streams.size() && m_Timestamps.begin() != m_Timestamp;i++)
  {
    --m_Timestamp;
  }
  return true;
}

DemuxPacket* CDVDDemuxVobsub::Read()
{
  vector<STimestamp>::iterator current;
  do {
    if(m_Timestamp == m_Timestamps.end())
      return NULL;

    current =  m_Timestamp++;
  } while(m_Streams[current->id]->m_discard == AVDISCARD_ALL);

  if(!m_Demuxer->SeekByte(current->pos))
    return NULL;

  DemuxPacket *packet = m_Demuxer->Read();
  if(!packet)
    return NULL;

  packet->iStreamId = current->id;
  packet->pts = current->pts;
  packet->dts = current->pts;

  return packet;
}

bool CDVDDemuxVobsub::ParseLangIdx(SState& state, char* line)
{
  return true;
}

bool CDVDDemuxVobsub::ParseDelay(SState& state, char* line)
{
  int h,m,s,ms;
  bool negative = false;

  while(*line == ' ') line++;
  if(*line == '-')
  {
	  line++;
	  negative = true;
  }
  if(sscanf(line, "%d:%d:%d:%d", &h, &m, &s, &ms) != 4)
    return false;
  state.delay = h*3600.0 + m*60.0 + s + ms*0.001;
  if(negative)
	  state.delay *= -1;
  return true;
}

bool CDVDDemuxVobsub::ParseId(SState& state, char* line)
{
  unique_ptr<CStream> stream(new CStream(this));

  while(*line == ' ') line++;
  strncpy(stream->language, line, 2);
  stream->language[2] = '\0';
  line+=2;

  while(*line == ' ' || *line == ',') line++;
  if (strncmp("index:", line, 6) == 0)
  {
    line+=6;
    while(*line == ' ') line++;
    stream->iPhysicalId = atoi(line);
  }
  else
    stream->iPhysicalId = -1;

  stream->codec = AV_CODEC_ID_DVD_SUBTITLE;
  stream->iId = m_Streams.size();
  stream->source = STREAM_SOURCE_DEMUX_SUB;

  state.id = stream->iId;
  m_Streams.push_back(stream.release());
  return true;
}

bool CDVDDemuxVobsub::ParseExtra(SState& state, char* line)
{
  state.extra += line;
  state.extra += '\n';
  return true;
}

bool CDVDDemuxVobsub::ParseTimestamp(SState& state, char* line)
{
  if(state.id < 0)
    return false;

  int h,m,s,ms;
  STimestamp timestamp;

  while(*line == ' ') line++;
  if(sscanf(line, "%d:%d:%d:%d, filepos:%" PRIx64, &h, &m, &s, &ms, &timestamp.pos) != 5)
    return false;

  timestamp.id  = state.id;
  timestamp.pts = DVD_SEC_TO_TIME(state.delay + h*3600.0 + m*60.0 + s + ms*0.001);
  m_Timestamps.push_back(timestamp);
  return true;
}
