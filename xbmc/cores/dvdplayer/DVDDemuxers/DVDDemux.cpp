
#include "../../../stdafx.h"
#include "DVDDemux.h"
#include "..\ffmpeg\ffmpeg.h"


void CDemuxStreamAudio::GetStreamType(std::string& strInfo)
{
  char sInfo[32];
  
  if (codec == CODEC_ID_AC3) strcpy(sInfo, "AC3 ");
  else if (codec == CODEC_ID_DTS) strcpy(sInfo, "DTS ");
    
  if (iChannels == 1) strcat(sInfo, "mono");
  else if (iChannels == 2) strcat(sInfo, "stereo");
  else if (iChannels == 6) strcat(sInfo, "5.1");
  else if (iChannels != 0)
  {
    char temp[32];
    sprintf(temp, " %d %s", iChannels, "channels");
    strcat(sInfo, temp);
  }
  strInfo = sInfo;
}

int CDVDDemux::GetNrOfAudioStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_AUDIO) iCounter++;
  }
  
  return iCounter;
}

int CDVDDemux::GetNrOfVideoStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_VIDEO) iCounter++;
  }
  
  return iCounter;
}

CDemuxStreamAudio* CDVDDemux::GetStreamFromAudioId(int iAudioIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_AUDIO) counter++;
    if (iAudioIndex == counter)
    {
      return (CDemuxStreamAudio*)pStream;
    }
  }
  return NULL;
}

CDemuxStreamVideo* CDVDDemux::GetStreamFromVideoId(int iVideoIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_VIDEO) counter++;
    if (iVideoIndex == counter)
    {
      return (CDemuxStreamVideo*)pStream;
    }
  }
  return NULL;
}