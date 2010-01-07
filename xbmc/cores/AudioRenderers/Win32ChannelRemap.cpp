#include "Win32ChannelRemap.h"
#include "Log.h"

CWin32ChannelRemap::CWin32ChannelRemap() :
  m_pChannelMap(NULL),
  m_uiChannels(0),
  m_uiBitsPerSample(0),
  m_bPassthrough(false)
{
}

CWin32ChannelRemap::~CWin32ChannelRemap()
{
}

// Channel maps
// Our output order is FL, FR, C, LFE, SL, SR, SC, FLC, FRC
const unsigned char ac3_51_Map[] = {0,1,4,5,2,3};    // Sent as FL, FR, SL, SR, C, LFE
const unsigned char ac3_50_Map[] = {0,1,4,2,3};      // Sent as FL, FR, SL, SR, C
const unsigned char eac3_51_Map[] = {0,2,1,4,5,3};   // Sent as FL, C, FR, SL, SR, LFE
const unsigned char eac3_50_Map[] = {0,2,1,4,5};     // Sent as FL, C, FR, SL, SR, LFE
const unsigned char aac_51_Map[] = {2,0,1,4,5,3};    // Sent as C, FL, FR, SL, SR, LFE
const unsigned char aac_50_Map[] = {2,0,1,4,5};      // Sent as C, FL, FR, SL, SR
const unsigned char vorbis_51_Map[] = {0,2,1,4,5,3}; // Sent as FL, C, FR, SL, SR, LFE
const unsigned char vorbis_50_Map[] = {0,2,1,4,5};   // Sent as FL, C, FR, SL, SR

void CWin32ChannelRemap::SetChannelMap(unsigned int channels, unsigned int bitsPerSample, bool passthrough, const char* strAudioCodec)
{
  if (!strcmp(strAudioCodec, "AC3") || !strcmp(strAudioCodec, "DTS"))
  {
    if (channels == 6)
      m_pChannelMap = (unsigned char*)ac3_51_Map;
    else if (channels == 5)
      m_pChannelMap = (unsigned char*)ac3_50_Map;
  }
  else if (!strcmp(strAudioCodec, "AAC"))
  {
    if (channels == 6)
      m_pChannelMap = (unsigned char*)aac_51_Map;
    else if (channels == 5)
      m_pChannelMap = (unsigned char*)aac_50_Map;
  }
  else if (!strcmp(strAudioCodec, "Vorbis"))
  {
    if (channels == 6)
      m_pChannelMap = (unsigned char*)vorbis_51_Map;
    else if (channels == 5)
      m_pChannelMap = (unsigned char*)vorbis_50_Map;
  }
  else if (!strcmp(strAudioCodec, "EAC3"))
  {
    if (channels == 6)
      m_pChannelMap = (unsigned char*)eac3_51_Map;
    else if (channels == 5)
      m_pChannelMap = (unsigned char*)eac3_50_Map;
  }
  else
  {
    m_pChannelMap = NULL;
  }

  m_uiChannels = channels;
  m_uiBitsPerSample = bitsPerSample;
  m_bPassthrough = passthrough;
}

void CWin32ChannelRemap::MapDataIntoBuffer(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  if(pData == NULL || pOut == NULL || len == 0)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Null data pointer or 0 length parameter passed.");
    return;
  }

  if (m_pChannelMap && !m_bPassthrough)
  {
    switch(m_uiBitsPerSample)
    {
    case 8:
      MapData8(pData, len, pOut);
      break;
    case 16:
      MapData16(pData, len, pOut);
      break;
    case 24:
      MapData24(pData, len, pOut);
      break;
    case 32:
      MapData32(pData, len, pOut);
      break;
    default:
      CLog::Log(LOGERROR, __FUNCTION__": Invalid bits per sample applied to channel mapping");
      break;
    }
  }
  else
  {
    memcpy(pOut, pData, len); // If no mapping required, just copy the data.
  }
}

void CWin32ChannelRemap::MapData8(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  unsigned char* pOutFrame = pOut;
  for (unsigned char* pInFrame = pData; pInFrame < pData + (len / sizeof(unsigned char)); pInFrame += m_uiChannels, pOutFrame += m_uiChannels)
  {
    // Remap a single frame
    for (unsigned int chan = 0; chan < m_uiChannels; chan++)
      pOutFrame[m_pChannelMap[chan]] = pInFrame[chan]; // Copy sample into correct position in the output buffer
  }
}

void CWin32ChannelRemap::MapData16(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  short* pOutFrame = (short*)pOut;
  for (short* pInFrame = (short*)pData; pInFrame < (short*)pData + (len / sizeof(short)); pInFrame += m_uiChannels, pOutFrame += m_uiChannels)
  {
    // Remap a single frame
    for (unsigned int chan = 0; chan < m_uiChannels; chan++)
      pOutFrame[m_pChannelMap[chan]] = pInFrame[chan]; // Copy sample into correct position in the output buffer
  }
}

void CWin32ChannelRemap::MapData24(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  unsigned char* pOutFrame = pOut;
  for (unsigned char* pInFrame = pData; pInFrame < pData + (len / sizeof(unsigned char)); pInFrame += m_uiChannels*3, pOutFrame += m_uiChannels*3)
  {
    // Remap a single frame
    for (unsigned int chan = 0; chan < m_uiChannels; chan++)
    {
      pOutFrame[m_pChannelMap[chan]*3] = pInFrame[chan*3]; // Copy sample into correct position in the output buffer
      pOutFrame[(m_pChannelMap[chan]*3)+1] = pInFrame[(chan*3)+1];
      pOutFrame[(m_pChannelMap[chan]*3)+2] = pInFrame[(chan*3)+2];
    }
  }
}

void CWin32ChannelRemap::MapData32(unsigned char* pData, unsigned int len, unsigned char* pOut)
{
  int* pOutFrame = (int*)pOut;
  for (int* pInFrame = (int*)pData; pInFrame < (int*)pData + (len / sizeof(int)); pInFrame += m_uiChannels, pOutFrame += m_uiChannels)
  {
    // Remap a single frame
    for (unsigned int chan = 0; chan < m_uiChannels; chan++)
      pOutFrame[m_pChannelMap[chan]] = pInFrame[chan]; // Copy sample into correct position in the output buffer
  }
}
