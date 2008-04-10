
#pragma once

#define MAX_EMULATED_FILES    50
#define FILE_WRAPPER_OFFSET   0x00000100

typedef struct stEmuFileObject
{
  bool    used;
  FILE    file_emu;
  XFILE::CFile*  file_xbmc;
} EmuFileObject;
  
class CEmuFileWrapper
{
public:
  CEmuFileWrapper();
  ~CEmuFileWrapper();
  
  /**
   * Only to be called when shutting down xbmc
   */
  void CleanUp();
  
  EmuFileObject* RegisterFileObject(XFILE::CFile* pFile);
  void UnRegisterFileObjectByDescriptor(int fd);
  void UnRegisterFileObjectByStream(FILE* stream);
  EmuFileObject* GetFileObjectByDescriptor(int fd);  
  EmuFileObject* GetFileObjectByStream(FILE* stream);  
  XFILE::CFile* GetFileXbmcByDescriptor(int fd);
  XFILE::CFile* GetFileXbmcByStream(FILE* stream);
  int GetDescriptorByStream(FILE* stream);
  FILE* GetStreamByDescriptor(int fd);
  bool DescriptorIsEmulatedFile(int fd);
  bool StreamIsEmulatedFile(FILE* stream);
private:
  EmuFileObject m_files[MAX_EMULATED_FILES];
  CRITICAL_SECTION m_criticalSection;
  bool m_initialized;
};

extern CEmuFileWrapper g_emuFileWrapper;

