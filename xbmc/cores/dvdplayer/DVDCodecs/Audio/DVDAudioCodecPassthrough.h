#pragma once
#include "DVDAudioCodec.h"
#include "DllLibDts.h"
#include "DllLiba52.h"


class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough();
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPasstrough() { return true; }
  virtual const char* GetName()  { return "passthrough"; }
  
private:
  bool SyncAC3Header(BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize );
  bool SyncDTSHeader( BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize );
  int PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut);
  int PaddDTSData( BYTE* pData, int iDataSize, BYTE* pOut);
  
  int m_iPassBufferAlloced;
  BYTE* m_pPassBuffer;
  int m_iPassBufferLen;

  bool m_bHasMMByte;
  BYTE m_bMMByte;
  
  BYTE* m_pDataFrame;
  int m_iDataFrameAlloced;
  int m_iDataFrameLen;
  int m_iOffset;

  int m_iFrameSize; //Input number of bytes  
  int m_iChunkSize;

  int m_iSamplesPerFrame; //Equivalent number of output bytes per frame
  int m_iSampleRate;

  enum EN_SYNCTYPE
  {
    ENS_UNKNOWN=0,
    ENS_AC3=1,
    ENS_DTS=2
  } m_iType;

  DllLibDts m_dllDTS;
  DllLiba52 m_dllA52;
  
  dts_state_t* m_pStateDTS;
  a52_state_t* m_pStateA52;
};
