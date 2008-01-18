#ifndef SPC_CODEC_H_
#define SPC_CODEC_H_

#include "ICodec.h"
#include "spc/Types.h"
#include "..\DllLoader\DllLoader.h"

class SPCCodec : public ICodec
{
public:
  SPCCodec();
  virtual ~SPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  typedef void  (__stdcall* LoadMethod) ( const void* p1);
  typedef void* (__stdcall * EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__stdcall * SeekMethod) ( u32 p1, b8 p2 );
  struct   
  {
    LoadMethod LoadSPCFile;
    EmuMethod EmuAPU;
    SeekMethod SeekAPU;
  } m_dll;

  DllLoader* m_loader;
  char* m_szBuffer;
  u8* m_pApuRAM;
  __int64 m_iDataPos;
};

#endif