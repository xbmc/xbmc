//------------------------------
// COggTag in 2003 by Bobbin007
//------------------------------
#include "VorbisTag.h"
#include "cores/paplayer/replaygain.h"
#include "cores/paplayer/ogg/vorbisfile.h"

namespace MUSIC_INFO
{

#pragma once

class COggTag : public CVorbisTag
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
  COggTag(void);
  virtual ~COggTag(void);
  virtual bool ReadTag(const CStdString& strFile);
          int  GetStreamCount(const CStdString& strFile);
protected:
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
  OGGdll m_dll;
};
};
