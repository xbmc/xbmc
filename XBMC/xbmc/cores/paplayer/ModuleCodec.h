#ifndef MODULE_CODEC_H_
#define MODULE_CODEC_H_

#include "ICodec.h"

class ModuleCodec : public ICodec
{
  struct ModuleDLL 
  {
    int (__cdecl *DLL_LoadModule)(const char* szFileName);
    void (__cdecl *DLL_FreeModule)(int);
    int (__cdecl *DLL_GetModuleLength)(int duh);
    int (__cdecl *DLL_GetModulePosition)(int sic);
    int (__cdecl *DLL_StartPlayback)(int duh, long pos);
    void (__cdecl *DLL_StopPlayback)(int sic);
    long (__cdecl *DLL_FillBuffer)(int, int sig, char* buffer, int size, float volume);
  };

public:
  ModuleCodec();
  virtual ~ModuleCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

  static bool IsSupportedFormat(const CStdString& strExt);

private:
  bool LoadDLL();
  
  long m_iDataLen;
  int m_module;
  int m_renderID;
  int m_iFilePos;
  bool m_bDllLoaded;

  ModuleDLL m_dll;
};

#endif