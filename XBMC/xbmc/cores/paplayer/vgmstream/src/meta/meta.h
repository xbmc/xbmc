#ifndef _META_H
#define _META_H

#include "../vgmstream.h"

VGMSTREAM * init_vgmstream_adx(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_afc(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_agsc(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ast(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_brstm(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_Cstr(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_gcsw(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_halpst(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_nds_strm(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_adpdtk(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_dsp_std(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_dsp_stm(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_mpdsp(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_dsp_std_int(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_ads(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_npsf(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_rs03(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_rsf(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_rwsd(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_cdxa(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_rxw(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_int(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_exst(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_svag(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_mib(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_mic(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_raw(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_vag(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_psx_gms(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_str(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_ild(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_pnb(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_xbox_wavm(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_xbox_xwav(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ngc_str(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ea(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_caf(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_vpk(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_genh(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_amts(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_xbox_stma(STREAMFILE *streamFile);

#ifdef VGM_USE_VORBIS
VGMSTREAM * init_vgmstream_ogg_vorbis(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_sli_ogg(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_sfl(STREAMFILE * streamFile);
#endif

VGMSTREAM * init_vgmstream_sadb(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_ps2_bmdx(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_wsi(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_aifc(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_str_snds(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ws_aud(STREAMFILE * streamFile);

#ifdef VGM_USE_MPEG
VGMSTREAM * init_vgmstream_ahx(STREAMFILE * streamFile);
#endif

VGMSTREAM * init_vgmstream_ivb(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_svs(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_riff(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_pos(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_nwa(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_eacs(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_xss(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_sl3(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_hgc1(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_aus(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_rws(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_rsd(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_fsb(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_rwx(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_xwb(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_xa30(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_musc(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_musx(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_leg(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_filp(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ikm(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_sfs(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_dvi(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_bg00(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_kcey(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_rstm(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_acm(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_kces(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_dxh(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_psh(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_mus_acm(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_pcm(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_rkv(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_psw(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_vas(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_tec(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ps2_enth(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_sdt(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_aix(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ngc_tydsp(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ngc_swd(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ngc_vjdsp(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_ngc_biodsp(STREAMFILE * streamFile);

VGMSTREAM * init_vgmstream_xbox_wvs(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_dc_str(STREAMFILE *streamFile);

VGMSTREAM * init_vgmstream_xbox_matx(STREAMFILE *streamFile);

#endif
