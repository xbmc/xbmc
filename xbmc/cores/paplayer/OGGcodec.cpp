#include "../../stdafx.h"
#include "OGGCodec.h"

//  Note: this dll has the ogg.dll and vorbis.dll statically linked 
#define OGG_DLL "Q:\\system\\players\\PAPlayer\\vorbisfile.dll"

size_t ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return pFile->Read(ptr, size*nmemb);
}

int ogg_seek(void *datasource, ogg_int64_t offset, int whence)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return (int)pFile->Seek(offset, whence);
}

int ogg_close(void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  pFile->Close();
  return 1;
}

long ogg_tell(void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return (long)pFile->GetPosition();
}

OGGCodec::OGGCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;

  // dll stuff
  ZeroMemory(&m_dll, sizeof(OGGdll));
  m_bDllLoaded = false;
}

OGGCodec::~OGGCodec()
{
  DeInit();
  CSectionLoader::UnloadDLL(OGG_DLL);
}

bool OGGCodec::Init(const CStdString &strFile)
{
  if (!LoadDLL())
    return false;

  if (!m_fileOGG.Open(strFile.c_str()))
    return false;

  //  setup ogg i/o callbacks
  ov_callbacks oggIOCallbacks;
  oggIOCallbacks.read_func=ogg_read;
  oggIOCallbacks.seek_func=ogg_seek;
  oggIOCallbacks.tell_func=ogg_tell;
  oggIOCallbacks.close_func=ogg_close;

  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks((void*)&m_fileOGG, &m_VorbisFile, NULL, 0, oggIOCallbacks)!=0)
    return false;

  //  get file info
  vorbis_info* pInfo=m_dll.ov_info(&m_VorbisFile, -1);
  m_SampleRate = pInfo->rate;
  m_Channels = pInfo->channels;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)m_dll.ov_time_total(&m_VorbisFile, -1)*1000;

  return true;
}

void OGGCodec::DeInit()
{
  m_dll.ov_clear(&m_VorbisFile);
}

__int64 OGGCodec::Seek(__int64 iSeekTime)
{
  //  Calculate the next full second...
  int iSeekTimeFullSec=(int)(iSeekTime+(1000-(iSeekTime%1000)))/1000;

  //  ...and seek to the new time
  if (m_dll.ov_time_seek(&m_VorbisFile, (double)iSeekTimeFullSec)!=0)
    return 0;

  return iSeekTimeFullSec*1000;
}

int OGGCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iBitStream=-1;
  long lReadNow=0;
  long lTotalRead=0;
  long iSizeLeft=size;
  long iAmountToRead= size<4096 ? size : 4096;

  //  Fill buffer as much as possible
  while (true)
  {
    //  the maximum chunk size the vorbis decoder seem to return with on call is 4096
    lReadNow=m_dll.ov_read(&m_VorbisFile, (char*)pBuffer+lTotalRead, iAmountToRead, 0, 2, 1, &iBitStream);

    if (lReadNow<0)
    {
      CLog::Log(LOGERROR, "OGGCodec: Read error %i", lReadNow);
      return READ_ERROR;
    }

    lTotalRead+=lReadNow;
    iSizeLeft-=lReadNow;

    //  Is our buffer filled as much as possible
    if ((lTotalRead==size || iSizeLeft/4096==0) || (lReadNow==0 && lTotalRead!=0))
    {
      *actualsize=lTotalRead;
      return READ_SUCCESS;
    }
    else if (lReadNow==0)
      return READ_EOF;
  }

  return READ_ERROR;
}

bool OGGCodec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll=CSectionLoader::LoadDLL(OGG_DLL);

  if (!pDll)
    return false;

  pDll->ResolveExport("ov_clear", (void**)&m_dll.ov_clear);
  pDll->ResolveExport("ov_open", (void**)&m_dll.ov_open);
  pDll->ResolveExport("ov_open_callbacks", (void**)&m_dll.ov_open_callbacks);

  pDll->ResolveExport("ov_test", (void**)&m_dll.ov_test);
  pDll->ResolveExport("ov_test_callbacks", (void**)&m_dll.ov_test_callbacks);
  pDll->ResolveExport("ov_test_open", (void**)&m_dll.ov_test_open);

  pDll->ResolveExport("ov_bitrate", (void**)&m_dll.ov_bitrate);
  pDll->ResolveExport("ov_bitrate_instant", (void**)&m_dll.ov_bitrate_instant);
  pDll->ResolveExport("ov_streams", (void**)&m_dll.ov_streams);
  pDll->ResolveExport("ov_seekable", (void**)&m_dll.ov_seekable);
  pDll->ResolveExport("ov_serialnumber", (void**)&m_dll.ov_serialnumber);

  pDll->ResolveExport("ov_raw_total", (void**)&m_dll.ov_raw_total);
  pDll->ResolveExport("ov_pcm_total", (void**)&m_dll.ov_pcm_total);
  pDll->ResolveExport("ov_time_total", (void**)&m_dll.ov_time_total);

  pDll->ResolveExport("ov_raw_seek", (void**)&m_dll.ov_raw_seek);
  pDll->ResolveExport("ov_pcm_seek", (void**)&m_dll.ov_pcm_seek);
  pDll->ResolveExport("ov_pcm_seek_page", (void**)&m_dll.ov_pcm_seek_page);
  pDll->ResolveExport("ov_time_seek", (void**)&m_dll.ov_time_seek);
  pDll->ResolveExport("ov_time_seek_page", (void**)&m_dll.ov_time_seek_page);

  pDll->ResolveExport("ov_raw_tell", (void**)&m_dll.ov_raw_tell);
  pDll->ResolveExport("ov_pcm_tell", (void**)&m_dll.ov_pcm_tell);
  pDll->ResolveExport("ov_time_tell", (void**)&m_dll.ov_time_tell);

  pDll->ResolveExport("ov_info", (void**)&m_dll.ov_info);
  pDll->ResolveExport("ov_comment", (void**)&m_dll.ov_comment);

  pDll->ResolveExport("ov_read", (void**)&m_dll.ov_read);

  // Check resolves
  if (!m_dll.ov_clear || !m_dll.ov_open || !m_dll.ov_open_callbacks || 
      !m_dll.ov_test || !m_dll.ov_test_callbacks || !m_dll.ov_test_open || 
      !m_dll.ov_bitrate || !m_dll.ov_bitrate_instant || !m_dll.ov_streams || 
      !m_dll.ov_seekable ||   !m_dll.ov_serialnumber || !m_dll.ov_raw_total || 
      !m_dll.ov_pcm_total || !m_dll.ov_time_total || !m_dll.ov_raw_seek || 
      !m_dll.ov_pcm_seek || !m_dll.ov_pcm_seek_page || !m_dll.ov_time_seek || 
      !m_dll.ov_time_seek_page || !m_dll.ov_raw_tell || !m_dll.ov_pcm_tell || 
      !m_dll.ov_time_tell || !m_dll.ov_info || !m_dll.ov_comment || !m_dll.ov_read) 
  {
    CLog::Log(LOGERROR, "OGGCodec: Unable to load our dll %s", OGG_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

bool OGGCodec::HandlesType(const char *type)
{
  return ( strcmp(type, "ogg") == 0 );
}