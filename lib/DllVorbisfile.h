#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if defined(_LINUX) || defined(__APPLE__) || defined(WIN32)
  // avoid unused symbol warnings from the static callbacks
  // defined in vorbisfile.h on 1.2.3 and above
  #define OV_EXCLUDE_STATIC_CALLBACKS

  #include <vorbis/vorbisfile.h>
  #include "utils/log.h"
#else
  #include "cdrip/oggvorbis/vorbisfile.h"
#endif
#include "DynamicDll.h"

//  Note: the vorbisfile.dll has the ogg.dll and vorbis.dll statically linked

class DllVorbisfileInterface
{
public:
    virtual ~DllVorbisfileInterface() {}
    virtual int ov_clear(OggVorbis_File *vf)=0;
    virtual int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes)=0;
    virtual int ov_open_callbacks(void *datasource, OggVorbis_File *vf,
                                  char *initial, long ibytes, ov_callbacks callbacks)=0;
    virtual int ov_test(FILE *f,OggVorbis_File *vf,char *initial,long ibytes)=0;
    virtual int ov_test_callbacks(void *datasource, OggVorbis_File *vf,
                                  char *initial, long ibytes, ov_callbacks callbacks)=0;
    virtual int ov_test_open(OggVorbis_File *vf)=0;
    virtual long ov_bitrate(OggVorbis_File *vf,int i)=0;
    virtual long ov_bitrate_instant(OggVorbis_File *vf)=0;
    virtual long ov_streams(OggVorbis_File *vf)=0;
    virtual long ov_seekable(OggVorbis_File *vf)=0;
    virtual long ov_serialnumber(OggVorbis_File *vf,int i)=0;
    virtual ogg_int64_t ov_raw_total(OggVorbis_File *vf,int i)=0;
    virtual ogg_int64_t ov_pcm_total(OggVorbis_File *vf,int i)=0;
    virtual double ov_time_total(OggVorbis_File *vf,int i)=0;
    virtual int ov_raw_seek(OggVorbis_File *vf,ogg_int64_t pos)=0;
    virtual int ov_pcm_seek(OggVorbis_File *vf,ogg_int64_t pos)=0;
    virtual int ov_pcm_seek_page(OggVorbis_File *vf,ogg_int64_t pos)=0;
    virtual int ov_time_seek(OggVorbis_File *vf,double pos)=0;
    virtual int ov_time_seek_page(OggVorbis_File *vf,double pos)=0;
    virtual ogg_int64_t ov_raw_tell(OggVorbis_File *vf)=0;
    virtual ogg_int64_t ov_pcm_tell(OggVorbis_File *vf)=0;
    virtual double ov_time_tell(OggVorbis_File *vf)=0;
    virtual vorbis_info *ov_info(OggVorbis_File *vf,int link)=0;
    virtual vorbis_comment *ov_comment(OggVorbis_File *vf,int link)=0;
    virtual long ov_read(OggVorbis_File *vf,char *buffer,int length,
                         int bigendianp,int word,int sgned,int *bitstream)=0;
};

class DllVorbisfile : public DllDynamic, DllVorbisfileInterface
{
  DECLARE_DLL_WRAPPER(DllVorbisfile, DLL_PATH_OGG_CODEC)
  DEFINE_METHOD1(int, ov_clear, (OggVorbis_File *p1))
  DEFINE_METHOD4(int, ov_open, (FILE *p1,OggVorbis_File *p2,char *p3,long p4))
  DEFINE_METHOD5(int, ov_open_callbacks, (void *p1, OggVorbis_File *p2, char *p3, long p4, ov_callbacks p5))
  DEFINE_METHOD4(int, ov_test, (FILE *p1,OggVorbis_File *p2,char *p3,long p4))
  DEFINE_METHOD5(int, ov_test_callbacks, (void *p1, OggVorbis_File *p2, char *p3, long p4, ov_callbacks p5))
  DEFINE_METHOD1(int, ov_test_open, (OggVorbis_File *p1))
  DEFINE_METHOD2(long, ov_bitrate, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD1(long, ov_bitrate_instant, (OggVorbis_File *p1))
  DEFINE_METHOD1(long, ov_streams, (OggVorbis_File *p1))
  DEFINE_METHOD1(long, ov_seekable, (OggVorbis_File *p1))
  DEFINE_METHOD2(long, ov_serialnumber, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD2(ogg_int64_t, ov_raw_total, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD2(ogg_int64_t, ov_pcm_total, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD2(double, ov_time_total, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD2(int, ov_raw_seek, (OggVorbis_File *p1,ogg_int64_t p2))
  DEFINE_METHOD2(int, ov_pcm_seek, (OggVorbis_File *p1,ogg_int64_t p2))
  DEFINE_METHOD2(int, ov_pcm_seek_page, (OggVorbis_File *p1,ogg_int64_t p2))
  DEFINE_METHOD2(int, ov_time_seek, (OggVorbis_File *p1,double p2))
  DEFINE_METHOD2(int, ov_time_seek_page, (OggVorbis_File *p1,double p2))
  DEFINE_METHOD1(ogg_int64_t, ov_raw_tell, (OggVorbis_File *p1))
  DEFINE_METHOD1(ogg_int64_t, ov_pcm_tell, (OggVorbis_File *p1))
  DEFINE_METHOD1(double, ov_time_tell, (OggVorbis_File *p1))
  DEFINE_METHOD2(vorbis_info *, ov_info, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD2(vorbis_comment *, ov_comment, (OggVorbis_File *p1,int p2))
  DEFINE_METHOD7(long, ov_read, (OggVorbis_File *p1,char *p2,int p3, int p4,int p5,int p6,int *p7))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(ov_clear)
    RESOLVE_METHOD(ov_open)
    RESOLVE_METHOD(ov_open_callbacks)
    RESOLVE_METHOD(ov_test)
    RESOLVE_METHOD(ov_test_callbacks)
    RESOLVE_METHOD(ov_test_open)
    RESOLVE_METHOD(ov_bitrate)
    RESOLVE_METHOD(ov_bitrate_instant)
    RESOLVE_METHOD(ov_streams)
    RESOLVE_METHOD(ov_seekable)
    RESOLVE_METHOD(ov_serialnumber)
    RESOLVE_METHOD(ov_raw_total)
    RESOLVE_METHOD(ov_pcm_total)
    RESOLVE_METHOD(ov_time_total)
    RESOLVE_METHOD(ov_raw_seek)
    RESOLVE_METHOD(ov_pcm_seek)
    RESOLVE_METHOD(ov_pcm_seek_page)
    RESOLVE_METHOD(ov_time_seek)
    RESOLVE_METHOD(ov_time_seek_page)
    RESOLVE_METHOD(ov_raw_tell)
    RESOLVE_METHOD(ov_pcm_tell)
    RESOLVE_METHOD(ov_time_tell)
    RESOLVE_METHOD(ov_info)
    RESOLVE_METHOD(ov_comment)
    RESOLVE_METHOD(ov_read)
  END_METHOD_RESOLVE()
};

