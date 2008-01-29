
#include "stdafx.h"
#include "DVDDemux.h"
#include "../DVDCodecs/DVDCodecs.h"
#include "LangCodeExpander.h"

void CDemuxStreamAudio::GetStreamType(std::string& strInfo)
{
  char sInfo[64];
  
  if (codec == CODEC_ID_AC3) strcpy(sInfo, "AC3 ");
  else if (codec == CODEC_ID_DTS) strcpy(sInfo, "DTS ");
  else if (codec == CODEC_ID_MP2) strcpy(sInfo, "MP2 ");
  else strcpy(sInfo, "");
    
  if (iChannels == 1) strcat(sInfo, "Mono");
  else if (iChannels == 2) strcat(sInfo, "Stereo");
  else if (iChannels == 6) strcat(sInfo, "5.1");
  else if (iChannels != 0)
  {
    char temp[32];
    sprintf(temp, " %d %s", iChannels, "Channels");
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

int CDVDDemux::GetNrOfSubtitleStreams()
{
  int iCounter = 0;

  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);
    if (pStream->type == STREAM_SUBTITLE) iCounter++;
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
      return (CDemuxStreamAudio*)pStream;
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
      return (CDemuxStreamVideo*)pStream;
  }
  return NULL;
}

CDemuxStreamSubtitle* CDVDDemux::GetStreamFromSubtitleId(int iSubtitleIndex)
{
  int counter = -1;
  for (int i = 0; i < GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = GetStream(i);

    if (pStream->type == STREAM_SUBTITLE) counter++;
    if (iSubtitleIndex == counter)
      return (CDemuxStreamSubtitle*)pStream;
  }
  return NULL;
}

void CDemuxStream::GetStreamName( std::string& strInfo )
{   
  if( language[0] == 0 )
    strInfo = "";
  else
  {
    CStdString name;
    g_LangCodeExpander.Lookup( name, language );
    strInfo = name;
  }
}

AVDiscard CDemuxStream::GetDiscard()
{
  return AVDISCARD_NONE;
}

void CDemuxStream::SetDiscard(AVDiscard discard)
{
  return;
}

