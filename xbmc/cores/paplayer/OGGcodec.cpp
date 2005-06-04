#include "../../stdafx.h"
#include "OGGCodec.h"
#include "../../oggtag.h"
#include "../../Util.h"

//  Note: the vorbisfile.dll has the ogg.dll and vorbis.dll statically linked 

OGGCodec::OGGCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_Bitrate = 0;
  m_CodecName = L"OGG";
  m_TimeOffset = 0;
  m_CurrentStream=0;
  m_VorbisFile.datasource = NULL;

  // dll stuff
  ZeroMemory(&m_dll, sizeof(OGGdll));
  m_bDllLoaded = false;
}

OGGCodec::~OGGCodec()
{
  DeInit();
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(OGG_DLL);
}

bool OGGCodec::Init(const CStdString &strFile1, unsigned int filecache)
{
  m_file.Initialize(filecache);

  CStdString strFile=strFile1;
  if (!LoadDLL())
    return false;
  
  m_CurrentStream=0;

  CStdString strExtension;
  CUtil::GetExtension(strFile, strExtension);

  //  A bitstream inside a ogg file?
  if (strExtension==".oggstream")
  {
    //  Extract the bitstream to play
    CStdString strFileName=CUtil::GetFileName(strFile);
    int iStart=strFileName.ReverseFind("-")+1;
    m_CurrentStream = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str())-1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFile;
    CUtil::GetDirectory(strPath, strFile);
    if (CUtil::HasSlashAtEnd(strFile))
      strFile.Delete(strFile.size()-1);
  }

  //  Open the file to play
  if (!m_file.Open(strFile.c_str()))
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't open %s", strFile1.c_str());
    return false;
  }

  //  setup ogg i/o callbacks
  ov_callbacks oggIOCallbacks;
  oggIOCallbacks.read_func=ReadCallback;
  oggIOCallbacks.seek_func=SeekCallback;
  oggIOCallbacks.tell_func=TellCallback;
  oggIOCallbacks.close_func=CloseCallback;

  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks(this, &m_VorbisFile, NULL, 0, oggIOCallbacks)!=0)
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't open decoder for %s", strFile1.c_str());
    return false;
  }

  long iStreams=m_dll.ov_streams(&m_VorbisFile);
  if (iStreams>1)
  {
    if (m_CurrentStream > iStreams)
      return false;
  }

  //  Calculate the offset in secs where the bitstream starts
  for (int i=0; i<m_CurrentStream; ++i)
    m_TimeOffset += (__int64)m_dll.ov_time_total(&m_VorbisFile, i);

  //  get file info
  vorbis_info* pInfo=m_dll.ov_info(&m_VorbisFile, m_CurrentStream);
  if (!pInfo)
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't get stream info from %s", strFile1.c_str());
    return false;
  }

  m_SampleRate = pInfo->rate;
  m_Channels = pInfo->channels;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)m_dll.ov_time_total(&m_VorbisFile, m_CurrentStream)*1000;
  m_Bitrate = pInfo->bitrate_nominal;
  if (m_Bitrate == 0)
	  m_Bitrate = (int)(m_file.GetLength()*8 / (m_TotalTime / 1000));

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0)
  {
    CLog::Log(LOGERROR, "OGGCodec: incomplete stream info from %s, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%i", strFile1.c_str(), m_SampleRate, m_Channels, m_BitsPerSample, m_TotalTime);
    return false;
  }

  //  Get replay gain tags
  vorbis_comment* pComments=m_dll.ov_comment(&m_VorbisFile, m_CurrentStream);
  if (pComments)
  {
    COggTag oggTag;
    for (int i=0; i < pComments->comments; ++i)
    {
      CStdString strTag=pComments->user_comments[i];
      CStdString strItem;
      g_charsetConverter.utf8ToStringCharset(strTag, strItem);
      oggTag.ParseTagEntry(strItem);
    }
    m_replayGain=oggTag.GetReplayGain();
  }

  //  Seek to the logical bitstream to play
  if (m_TimeOffset>0)
  {
    if (m_dll.ov_time_seek_page(&m_VorbisFile, m_TimeOffset)!=0)
    {
      CLog::Log(LOGERROR, "OGGCodec: Can't seek to the bitstream start time (%s)", strFile1.c_str());
      return false;
    }
  }

  return true;
}

void OGGCodec::DeInit()
{
  if (m_VorbisFile.datasource)
    m_dll.ov_clear(&m_VorbisFile);
  m_VorbisFile.datasource = NULL;
}

__int64 OGGCodec::Seek(__int64 iSeekTime)
{
  if (m_dll.ov_time_seek(&m_VorbisFile, m_TimeOffset+(double)(iSeekTime/1000.0f))!=0)
    return 0;

  return iSeekTime;
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
    //  the maximum chunk size the vorbis decoder seem to return with one call is 4096
    lReadNow=m_dll.ov_read(&m_VorbisFile, (char*)pBuffer+lTotalRead, iAmountToRead, 0, 2, 1, &iBitStream);
    
    //  Our logical bitstream changed, we reached the eof
    if (m_CurrentStream!=iBitStream)
      lReadNow=0;

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
  {
    CLog::Log(LOGERROR, "OGGCodec: Unable to load dll %s", OGG_DLL);
    return false;
  }

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
    CLog::Log(LOGERROR, "OGGCodec: Unable to resolve exports from %s", OGG_DLL);
    CSectionLoader::UnloadDLL(OGG_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

bool OGGCodec::HandlesType(const char *type)
{
  return ( strcmp(type, "ogg") == 0 );
}

size_t OGGCodec::ReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  OGGCodec* pCodec=(OGGCodec*)datasource;
  if (!pCodec)
    return 0;

  return pCodec->m_file.Read(ptr, size*nmemb);
}

int OGGCodec::SeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
  OGGCodec* pCodec=(OGGCodec*)datasource;
  if (!pCodec)
    return 0;

  return (int)pCodec->m_file.Seek(offset, whence);
}

int OGGCodec::CloseCallback(void *datasource)
{
  OGGCodec* pCodec=(OGGCodec*)datasource;
  if (!pCodec)
    return 0;

  pCodec->m_file.Close();
  return 1;
}

long OGGCodec::TellCallback(void *datasource)
{
  OGGCodec* pCodec=(OGGCodec*)datasource;
  if (!pCodec)
    return 0;

  return (long)pCodec->m_file.GetPosition();
}
