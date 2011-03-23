#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined WIN32)
  #include "cdrip/oggvorbis/vorbisenc.h"
#else
  #include <vorbis/vorbisenc.h>
  #include "utils/log.h"
#endif
#include "DynamicDll.h"

class DllVorbisInterface
{
public:
  virtual void vorbis_info_init(vorbis_info *vi)=0;
  virtual int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op)=0;
  virtual int vorbis_bitrate_addblock(vorbis_block *vb)=0;
  virtual int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi)=0;
  virtual int vorbis_analysis(vorbis_block *vb, ogg_packet *op)=0;
  virtual int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb)=0;
  virtual int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals)=0;
  virtual float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals)=0;
  virtual int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code)=0;
  virtual int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb)=0;
  virtual int vorbis_block_clear(vorbis_block *vb)=0;
  virtual void vorbis_comment_add_tag(vorbis_comment *vc, char *tag, char *contents)=0;
  virtual void vorbis_comment_init(vorbis_comment *vc)=0;
  virtual void vorbis_info_clear(vorbis_info *vi)=0;
  virtual void vorbis_comment_clear(vorbis_comment *vc)=0;
  virtual void vorbis_dsp_clear(vorbis_dsp_state *v)=0;
  virtual ~DllVorbisInterface() {}
};

#if !(defined(WIN32) || defined(__APPLE__))

class DllVorbis : public DllDynamic, DllVorbisInterface
{
public:
    virtual ~DllVorbis() {};
    virtual void vorbis_info_init(vorbis_info *vi)
        { vorbis_info_init(vi); }
    virtual int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op)
        { return ::vorbis_bitrate_flushpacket(vd, op); }
    virtual int vorbis_bitrate_addblock(vorbis_block *vb)
        { return ::vorbis_bitrate_addblock(vb); }
    virtual int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi)
        { return ::vorbis_analysis_init(v, vi); }
    virtual int vorbis_analysis(vorbis_block *vb, ogg_packet *op)
        { return ::vorbis_analysis(vb, op); }
    virtual int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb)
        { return ::vorbis_analysis_blockout(v, vb); }
    virtual int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals)
        { return ::vorbis_analysis_wrote(v, vals); }
    virtual float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals)
        { return ::vorbis_analysis_buffer(v, vals); }
    virtual int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code)
        { return ::vorbis_analysis_headerout(v, vc, op, op_comm, op_code); }
    virtual int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb)
        { return ::vorbis_block_init(v, vb); }
    virtual int vorbis_block_clear(vorbis_block *vb)
        { return ::vorbis_block_clear(vb); }
    virtual void vorbis_comment_add_tag(vorbis_comment *vc, char *tag, char *contents)
        { return ::vorbis_comment_add_tag(vc, tag, contents); }
    virtual void vorbis_comment_init(vorbis_comment *vc)
        { return ::vorbis_comment_init(vc); }
    virtual void vorbis_info_clear(vorbis_info *vi)
        { return ::vorbis_info_clear(vi); }
    virtual void vorbis_comment_clear(vorbis_comment *vc)
        { return ::vorbis_comment_clear(vc); }
    virtual void vorbis_dsp_clear(vorbis_dsp_state *v)
        { return ::vorbis_dsp_clear(v); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllVorbis: Using libvorbis system library");
        return true;
    }
    virtual void Unload() {}
};

#else

class DllVorbis : public DllDynamic, DllVorbisInterface
{
  DECLARE_DLL_WRAPPER(DllVorbis, DLL_PATH_VORBIS)
  DEFINE_METHOD1(void, vorbis_info_init, (vorbis_info *p1))
  DEFINE_METHOD2(int, vorbis_bitrate_flushpacket, (vorbis_dsp_state *p1, ogg_packet *p2))
  DEFINE_METHOD1(int, vorbis_bitrate_addblock, (vorbis_block *p1))
  DEFINE_METHOD2(int, vorbis_analysis_init, (vorbis_dsp_state *p1, vorbis_info *p2))
  DEFINE_METHOD2(int, vorbis_analysis, (vorbis_block *p1, ogg_packet *p2))
  DEFINE_METHOD2(int, vorbis_analysis_blockout, (vorbis_dsp_state *p1, vorbis_block *p2))
  DEFINE_METHOD2(int, vorbis_analysis_wrote, (vorbis_dsp_state *p1, int p2))
  DEFINE_METHOD2(float**, vorbis_analysis_buffer, (vorbis_dsp_state *p1, int p2))
  DEFINE_METHOD5(int, vorbis_analysis_headerout, (vorbis_dsp_state *p1, vorbis_comment *p2, ogg_packet *p3, ogg_packet *p4, ogg_packet *p5))
  DEFINE_METHOD2(int, vorbis_block_init, (vorbis_dsp_state *p1, vorbis_block *p2))
  DEFINE_METHOD1(int, vorbis_block_clear, (vorbis_block *p1))
  DEFINE_METHOD3(void, vorbis_comment_add_tag, (vorbis_comment *p1, char *p2, char *p3))
  DEFINE_METHOD1(void, vorbis_comment_init, (vorbis_comment *p1))
  DEFINE_METHOD1(void, vorbis_info_clear, (vorbis_info *p1))
  DEFINE_METHOD1(void, vorbis_comment_clear, (vorbis_comment *p1))
  DEFINE_METHOD1(void, vorbis_dsp_clear, (vorbis_dsp_state *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(vorbis_info_init)
    RESOLVE_METHOD(vorbis_bitrate_flushpacket)
    RESOLVE_METHOD(vorbis_bitrate_addblock)
    RESOLVE_METHOD(vorbis_analysis_init)
    RESOLVE_METHOD(vorbis_analysis)
    RESOLVE_METHOD(vorbis_analysis_blockout)
    RESOLVE_METHOD(vorbis_analysis_wrote)
    RESOLVE_METHOD(vorbis_analysis_buffer)
    RESOLVE_METHOD(vorbis_analysis_headerout)
    RESOLVE_METHOD(vorbis_block_init)
    RESOLVE_METHOD(vorbis_block_clear)
    RESOLVE_METHOD(vorbis_comment_add_tag)
    RESOLVE_METHOD(vorbis_comment_init)
    RESOLVE_METHOD(vorbis_info_clear)
    RESOLVE_METHOD(vorbis_comment_clear)
    RESOLVE_METHOD(vorbis_dsp_clear)
  END_METHOD_RESOLVE()
};

#endif
