#pragma once
#include "dvdaudiocodec.h"
#include "dvdaudiocodecliba52.h"

class CDVDAudioCodecPassthrough :
  public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough(void);
  ~CDVDAudioCodecPassthrough(void);

  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPasstrough() {return true;};
private:

  bool SyncDTSHeader( BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize );
  int PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut);

  BYTE* m_pPassBuffer;
  BYTE mPassBuffer[6144];
  bool m_bHasMMByte;
  BYTE m_bMMByte;
  
  BYTE* m_pDataFrame;
  int m_iDataFrameAlloced;
  int m_iDataFrameLen;
  int m_iOffset;

  int m_iFrameSize;
  int m_iChunkSize;


  int m_iPassBufferLen;

  enum EN_SYNCTYPE
  {
    ENS_UNKNOWN=0,
    ENS_AC3=1,
    ENS_DTS=2,
  } m_iType;

  CDVDAudioCodecLiba52* m_pLiba52;


};
