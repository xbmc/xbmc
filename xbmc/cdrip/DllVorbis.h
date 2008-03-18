#pragma once
#include "../DynamicDll.h"
#include "../cdrip/oggvorbis/vorbisenc.h"

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

class DllVorbis : public DllDynamic, DllVorbisInterface
{
#ifdef _LINUX
  DECLARE_DLL_WRAPPER(DllVorbis, Q:\\system\\cdrip\\vorbis-i486-linux.so)
#else
  DECLARE_DLL_WRAPPER(DllVorbis, Q:\\system\\cdrip\\vorbis.dll)
#endif
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
