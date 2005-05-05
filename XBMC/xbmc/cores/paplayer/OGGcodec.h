#pragma once
#include "ICodec.h"
#include "ogg/vorbisfile.h"

class OGGCodec : public ICodec
{
  struct OGGdll
  {
    int (__cdecl* ov_clear)(OggVorbis_File *vf);
    int (__cdecl* ov_open)(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);
    int (__cdecl* ov_open_callbacks)(void *datasource, OggVorbis_File *vf,
		                      char *initial, long ibytes, ov_callbacks callbacks);

    int (__cdecl* ov_test)(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);
    int (__cdecl* ov_test_callbacks)(void *datasource, OggVorbis_File *vf,
		                      char *initial, long ibytes, ov_callbacks callbacks);
    int (__cdecl* ov_test_open)(OggVorbis_File *vf);

    long (__cdecl* ov_bitrate)(OggVorbis_File *vf,int i);
    long (__cdecl* ov_bitrate_instant)(OggVorbis_File *vf);
    long (__cdecl* ov_streams)(OggVorbis_File *vf);
    long (__cdecl* ov_seekable)(OggVorbis_File *vf);
    long (__cdecl* ov_serialnumber)(OggVorbis_File *vf,int i);

    ogg_int64_t (__cdecl* ov_raw_total)(OggVorbis_File *vf,int i);
    ogg_int64_t (__cdecl* ov_pcm_total)(OggVorbis_File *vf,int i);
    double (__cdecl* ov_time_total)(OggVorbis_File *vf,int i);

    int (__cdecl* ov_raw_seek)(OggVorbis_File *vf,ogg_int64_t pos);
    int (__cdecl* ov_pcm_seek)(OggVorbis_File *vf,ogg_int64_t pos);
    int (__cdecl* ov_pcm_seek_page)(OggVorbis_File *vf,ogg_int64_t pos);
    int (__cdecl* ov_time_seek)(OggVorbis_File *vf,double pos);
    int (__cdecl* ov_time_seek_page)(OggVorbis_File *vf,double pos);

    ogg_int64_t (__cdecl* ov_raw_tell)(OggVorbis_File *vf);
    ogg_int64_t (__cdecl* ov_pcm_tell)(OggVorbis_File *vf);
    double (__cdecl* ov_time_tell)(OggVorbis_File *vf);

    vorbis_info *(__cdecl* ov_info)(OggVorbis_File *vf,int link);
    vorbis_comment *(__cdecl* ov_comment)(OggVorbis_File *vf,int link);

    long (__cdecl* ov_read)(OggVorbis_File *vf,char *buffer,int length,
		    int bigendianp,int word,int sgned,int *bitstream);
  };

public:
  OGGCodec();
  virtual ~OGGCodec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);

private:
  CFile m_fileOGG;
  OGGdll m_dll;
  OggVorbis_File m_VorbisFile;
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
};
