#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vgmstream.h"
#include "meta/meta.h"
#include "layout/layout.h"
#include "coding/coding.h"

/*
 * List of functions that will recognize files. These should correspond pretty
 * directly to the metadata types
 */
VGMSTREAM * (*init_vgmstream_fcns[])(STREAMFILE *streamFile) = {
    init_vgmstream_adx,
    init_vgmstream_brstm,
    init_vgmstream_nds_strm,
    init_vgmstream_agsc,
    init_vgmstream_ngc_adpdtk,
    init_vgmstream_rsf,
    init_vgmstream_afc,
    init_vgmstream_ast,
    init_vgmstream_halpst,
    init_vgmstream_rs03,
    init_vgmstream_ngc_dsp_std,
    init_vgmstream_Cstr,
    init_vgmstream_gcsw,
    init_vgmstream_ps2_ads,
    init_vgmstream_ps2_npsf,
    init_vgmstream_rwsd,
    init_vgmstream_cdxa,
    init_vgmstream_ps2_rxw,
    init_vgmstream_ps2_int,
    init_vgmstream_ngc_dsp_stm,
    init_vgmstream_ps2_exst,
    init_vgmstream_ps2_svag,
    init_vgmstream_ps2_mib,
    init_vgmstream_ngc_mpdsp,
    init_vgmstream_ps2_mic,
    init_vgmstream_ngc_dsp_std_int,
    init_vgmstream_raw,
    init_vgmstream_ps2_vag,
    init_vgmstream_psx_gms,
    init_vgmstream_ps2_str,
    init_vgmstream_ps2_ild,
    init_vgmstream_ps2_pnb,
    init_vgmstream_xbox_wavm,
    init_vgmstream_xbox_xwav,
    init_vgmstream_ngc_str,
    init_vgmstream_ea,
    init_vgmstream_caf,
    init_vgmstream_ps2_vpk,
    init_vgmstream_genh,
#ifdef VGM_USE_VORBIS
    init_vgmstream_ogg_vorbis,
    init_vgmstream_sli_ogg,
    init_vgmstream_sfl,
#endif
    init_vgmstream_sadb,
    init_vgmstream_ps2_bmdx,
    init_vgmstream_wsi,
    init_vgmstream_aifc,
    init_vgmstream_str_snds,
    init_vgmstream_ws_aud,
#ifdef VGM_USE_MPEG
    init_vgmstream_ahx,
#endif
    init_vgmstream_ivb,
    init_vgmstream_amts,
    init_vgmstream_svs,
    init_vgmstream_riff,
    init_vgmstream_pos,
    init_vgmstream_nwa,
    init_vgmstream_eacs,
    init_vgmstream_xss,
    init_vgmstream_sl3,
    init_vgmstream_hgc1,
    init_vgmstream_aus,
    init_vgmstream_rws,
    init_vgmstream_fsb1,
	init_vgmstream_fsb3,
	init_vgmstream_fsb4,
    init_vgmstream_rwx,
    init_vgmstream_xwb,
    init_vgmstream_xa30,
    init_vgmstream_musc,
    init_vgmstream_musx_v004,
	init_vgmstream_musx_v006,
	init_vgmstream_musx_v010,
	init_vgmstream_musx_v201,
    init_vgmstream_leg,
    init_vgmstream_filp,
    init_vgmstream_ikm,
    init_vgmstream_sfs,
    init_vgmstream_bg00,
    init_vgmstream_dvi,
    init_vgmstream_kcey,
    init_vgmstream_ps2_rstm,
    init_vgmstream_acm,
    init_vgmstream_mus_acm,
    init_vgmstream_ps2_kces,
    init_vgmstream_ps2_dxh,
    init_vgmstream_ps2_psh,
    init_vgmstream_pcm,
    init_vgmstream_ps2_rkv,
    init_vgmstream_ps2_psw,
    init_vgmstream_ps2_vas,
    init_vgmstream_ps2_tec,
    init_vgmstream_ps2_enth,
    init_vgmstream_sdt,
    init_vgmstream_aix,
    init_vgmstream_ngc_tydsp,
    init_vgmstream_ngc_swd,
    init_vgmstream_capdsp,
    init_vgmstream_xbox_wvs,
    init_vgmstream_dc_str,
    init_vgmstream_dc_str_v2,
    init_vgmstream_xbox_stma,
    init_vgmstream_xbox_matx,
    init_vgmstream_de2,
	init_vgmstream_vs,
    init_vgmstream_dc_str,
    init_vgmstream_dc_str_v2,
    init_vgmstream_xbox_xmu,
    init_vgmstream_xbox_xvas,
    init_vgmstream_ngc_bh2pcm,
    init_vgmstream_sat_sap,
    init_vgmstream_dc_idvi,
    init_vgmstream_ps2_rnd,
    init_vgmstream_wii_idsp,
	init_vgmstream_kraw,
	init_vgmstream_ps2_omu,
	init_vgmstream_ps2_xa2,
	init_vgmstream_idsp,
	init_vgmstream_idsp2,
	init_vgmstream_ngc_ymf,
    init_vgmstream_sadl,
	init_vgmstream_ps2_ccc,
	init_vgmstream_psx_fag,
	init_vgmstream_ps2_mihb,
	init_vgmstream_ngc_pdt,
    init_vgmstream_wii_mus,
	init_vgmstream_dc_asd,
	init_vgmstream_naomi_spsd,

	init_vgmstream_rsd2vag,
	init_vgmstream_rsd2pcmb,
	init_vgmstream_rsd2xadp,
	init_vgmstream_rsd3pcm,
	init_vgmstream_rsd4pcmb,
	init_vgmstream_rsd4pcm,
	init_vgmstream_rsd4vag,
	init_vgmstream_rsd6vag,
	init_vgmstream_rsd6wadp,
	init_vgmstream_rsd6xadp,
    init_vgmstream_bgw,
    init_vgmstream_spw,
	init_vgmstream_ps2_ass,
	init_vgmstream_waa_wac_wad_wam,
	init_vgmstream_ps2_seg,
	init_vgmstream_nds_strm_ffta2,
	init_vgmstream_str_asr,
	init_vgmstream_zwdsp,
	init_vgmstream_gca,
	init_vgmstream_spt_spd,
	init_vgmstream_ish_isd,
	init_vgmstream_gsp_gsb,
	init_vgmstream_ydsp,
	init_vgmstream_msvp,
	init_vgmstream_ngc_ssm,
	init_vgmstream_ps2_joe,
	init_vgmstream_vgs,
	init_vgmstream_dc_wav_dcs,
	init_vgmstream_wii_smp,
	init_vgmstream_emff_ps2,
	init_vgmstream_emff_ngc,
	init_vgmstream_ss_stream,
	init_vgmstream_thp,
	init_vgmstream_wii_sts,
	init_vgmstream_ps2_p2bt,
	init_vgmstream_ps2_gbts,
    init_vgmstream_wii_sng,
//	init_vgmstream_iadp,
    init_vgmstream_aax,
    init_vgmstream_ngc_ffcc_str,
};

#define INIT_VGMSTREAM_FCNS (sizeof(init_vgmstream_fcns)/sizeof(init_vgmstream_fcns[0]))

/* internal version with all parameters */
VGMSTREAM * init_vgmstream_internal(STREAMFILE *streamFile, int do_dfs) {
    int i;
    
    if (!streamFile)
        return NULL;

    /* try a series of formats, see which works */
    for (i=0;i<INIT_VGMSTREAM_FCNS;i++) {
        VGMSTREAM * vgmstream = (init_vgmstream_fcns[i])(streamFile);
        if (vgmstream) {
            /* these are little hacky checks */

            /* everything should have a reasonable sample rate
             * (a verification of the metadata) */
            if (!check_sample_rate(vgmstream->sample_rate)) {
                close_vgmstream(vgmstream);
                continue;
            }

            /* dual file stereo */
            if (do_dfs && (
                        (vgmstream->meta_type == meta_DSP_STD) ||
                        (vgmstream->meta_type == meta_PS2_VAGp) ||
                        (vgmstream->meta_type == meta_GENH) ||
                        (vgmstream->meta_type == meta_KRAW)
                        ) && vgmstream->channels == 1) {
                try_dual_file_stereo(vgmstream, streamFile);
            }

            /* save start things so we can restart for seeking */
            /* copy the channels */
            memcpy(vgmstream->start_ch,vgmstream->ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);
            /* copy the whole VGMSTREAM */
            memcpy(vgmstream->start_vgmstream,vgmstream,sizeof(VGMSTREAM));

            return vgmstream;
        }
    }

    return NULL;
}

/* format detection and VGMSTREAM setup, uses default parameters */
VGMSTREAM * init_vgmstream(const char * const filename) {
    VGMSTREAM *vgmstream = NULL;
    STREAMFILE *streamFile = open_stdio_streamfile(filename);
    if (streamFile) {
        vgmstream = init_vgmstream_from_STREAMFILE(streamFile);
        close_streamfile(streamFile);
    }
    return vgmstream;
}

VGMSTREAM * init_vgmstream_from_STREAMFILE(STREAMFILE *streamFile) {
    return init_vgmstream_internal(streamFile,1);
}

/* Reset a VGMSTREAM to its state at the start of playback.
 * Note that this does not reset the constituent STREAMFILES. */
void reset_vgmstream(VGMSTREAM * vgmstream) {
    /* copy the vgmstream back into itself */
    memcpy(vgmstream,vgmstream->start_vgmstream,sizeof(VGMSTREAM));

    /* copy the initial channels */
    memcpy(vgmstream->ch,vgmstream->start_ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);

    /* loop_ch is not zeroed here because there is a possibility of the
     * init_vgmstream_* function doing something tricky and precomputing it.
     * Otherwise hit_loop will be 0 and it will be copied over anyway when we
     * really hit the loop start. */

#ifdef VGM_USE_VORBIS
    if (vgmstream->coding_type==coding_ogg_vorbis) {
        ogg_vorbis_codec_data *data = vgmstream->codec_data;

        OggVorbis_File *ogg_vorbis_file = &(data->ogg_vorbis_file);

        ov_pcm_seek(ogg_vorbis_file, 0);
    }
#endif
#ifdef VGM_USE_MPEG
    if (vgmstream->layout_type==layout_mpeg ||
        vgmstream->layout_type==layout_fake_mpeg) {
        off_t input_offset;
        mpeg_codec_data *data = vgmstream->codec_data;

        /* input_offset is ignored as we can assume it will be 0 for a seek
         * to sample 0 */
        mpg123_feedseek(data->m,0,SEEK_SET,&input_offset);
        data->buffer_full = data->buffer_used = 0;
    }
#endif

    if (vgmstream->coding_type==coding_ACM) {
        mus_acm_codec_data *data = vgmstream->codec_data;
        int i;

        data->current_file = 0;
        for (i=0;i<data->file_count;i++) {
            acm_reset(data->files[i]);
        }
    }

    if (vgmstream->layout_type==layout_aix) {
        aix_codec_data *data = vgmstream->codec_data;
        int i;

        data->current_segment = 0;
        for (i=0;i<data->segment_count*data->stream_count;i++)
        {
            reset_vgmstream(data->adxs[i]);
        }
    }

    if (vgmstream->layout_type==layout_aax) {
        aax_codec_data *data = vgmstream->codec_data;
        int i;

        data->current_segment = 0;
        for (i=0;i<data->segment_count;i++)
        {
            reset_vgmstream(data->adxs[i]);
        }
    }

    if (
            vgmstream->coding_type == coding_NWA0 ||
            vgmstream->coding_type == coding_NWA1 ||
            vgmstream->coding_type == coding_NWA2 ||
            vgmstream->coding_type == coding_NWA3 ||
            vgmstream->coding_type == coding_NWA4 ||
            vgmstream->coding_type == coding_NWA5
       ) {
        nwa_codec_data *data = vgmstream->codec_data;
        reset_nwa(data->nwa);
    }
}

/* simply allocate memory for the VGMSTREAM and its channels */
VGMSTREAM * allocate_vgmstream(int channel_count, int looped) {
    VGMSTREAM * vgmstream;
    VGMSTREAM * start_vgmstream;
    VGMSTREAMCHANNEL * channels;
    VGMSTREAMCHANNEL * start_channels;
    VGMSTREAMCHANNEL * loop_channels;

    if (channel_count <= 0) return NULL;

    vgmstream = calloc(1,sizeof(VGMSTREAM));
    if (!vgmstream) return NULL;
    
    vgmstream->ch = NULL;
    vgmstream->start_ch = NULL;
    vgmstream->loop_ch = NULL;
    vgmstream->start_vgmstream = NULL;
    vgmstream->codec_data = NULL;

    start_vgmstream = calloc(1,sizeof(VGMSTREAM));
    if (!start_vgmstream) {
        free(vgmstream);
        return NULL;
    }
    vgmstream->start_vgmstream = start_vgmstream;
    start_vgmstream->start_vgmstream = start_vgmstream;

    channels = calloc(channel_count,sizeof(VGMSTREAMCHANNEL));
    if (!channels) {
        free(vgmstream);
        free(start_vgmstream);
        return NULL;
    }
    vgmstream->ch = channels;
    vgmstream->channels = channel_count;

    start_channels = calloc(channel_count,sizeof(VGMSTREAMCHANNEL));
    if (!start_channels) {
        free(vgmstream);
        free(start_vgmstream);
        free(channels);
        return NULL;
    }
    vgmstream->start_ch = start_channels;

    if (looped) {
        loop_channels = calloc(channel_count,sizeof(VGMSTREAMCHANNEL));
        if (!loop_channels) {
            free(vgmstream);
            free(start_vgmstream);
            free(channels);
            free(start_channels);
            return NULL;
        }
        vgmstream->loop_ch = loop_channels;
    }

    vgmstream->loop_flag = looped;

    return vgmstream;
}

void close_vgmstream(VGMSTREAM * vgmstream) {
    int i,j;
    if (!vgmstream) return;

#ifdef VGM_USE_VORBIS
    if (vgmstream->coding_type==coding_ogg_vorbis) {
        ogg_vorbis_codec_data *data = vgmstream->codec_data;
        if (vgmstream->codec_data) {
            OggVorbis_File *ogg_vorbis_file = &(data->ogg_vorbis_file);

            ov_clear(ogg_vorbis_file);

            close_streamfile(data->ov_streamfile.streamfile);
            free(vgmstream->codec_data);
            vgmstream->codec_data = NULL;
        }
    }
#endif

#ifdef VGM_USE_MPEG
    if (vgmstream->layout_type==layout_fake_mpeg||
        vgmstream->layout_type==layout_mpeg) {
        mpeg_codec_data *data = vgmstream->codec_data;

        if (data) {
            mpg123_delete(data->m);
            free(vgmstream->codec_data);
            vgmstream->codec_data = NULL;
            /* The astute reader will note that a call to mpg123_exit is never
             * made. While is is evilly breaking our contract with mpg123, it
             * doesn't actually do anything except set the "initialized" flag
             * to 0. And if we exit we run the risk of turning it off when
             * someone else in another thread is using it. */
        }
    }
#endif

    if (vgmstream->coding_type==coding_ACM) {
        mus_acm_codec_data *data = vgmstream->codec_data;

        if (data) {
            if (data->files) {
                int i;
                for (i=0; i<data->file_count; i++) {
                    /* shouldn't be duplicates */
                    if (data->files[i]) {
                        acm_close(data->files[i]);
                        data->files[i] = NULL;
                    }
                }
                free(data->files);
                data->files = NULL;
            }

            free(vgmstream->codec_data);
            vgmstream->codec_data = NULL;
        }
    }

    if (vgmstream->layout_type==layout_aix) {
        aix_codec_data *data = vgmstream->codec_data;

        if (data) {
            if (data->adxs) {
                int i;
                for (i=0;i<data->segment_count*data->stream_count;i++) {

                    /* note that the AIX close_streamfile won't do anything but
                     * deallocate itself, there is only one open file and that
                     * is in vgmstream->ch[0].streamfile  */
                    close_vgmstream(data->adxs[i]);
                }
                free(data->adxs);
            }
            if (data->sample_counts) {
                free(data->sample_counts);
            }

            free(data);
        }
        vgmstream->codec_data = NULL;
    }
    if (vgmstream->layout_type==layout_aax) {
        aax_codec_data *data = vgmstream->codec_data;

        if (data) {
            if (data->adxs) {
                int i;
                for (i=0;i<data->segment_count;i++) {

                    /* note that the AAX close_streamfile won't do anything but
                     * deallocate itself, there is only one open file and that
                     * is in vgmstream->ch[0].streamfile  */
                    close_vgmstream(data->adxs[i]);
                }
                free(data->adxs);
            }
            if (data->sample_counts) {
                free(data->sample_counts);
            }

            free(data);
        }
        vgmstream->codec_data = NULL;
    }

    if (
            vgmstream->coding_type == coding_NWA0 ||
            vgmstream->coding_type == coding_NWA1 ||
            vgmstream->coding_type == coding_NWA2 ||
            vgmstream->coding_type == coding_NWA3 ||
            vgmstream->coding_type == coding_NWA4 ||
            vgmstream->coding_type == coding_NWA5
       ) {
        nwa_codec_data *data = vgmstream->codec_data;

        close_nwa(data->nwa);
        
        free(data);

        vgmstream->codec_data = NULL;
    }

    /* now that the special cases have had their chance, clean up the standard items */
    for (i=0;i<vgmstream->channels;i++) {
        if (vgmstream->ch[i].streamfile) {
            close_streamfile(vgmstream->ch[i].streamfile);
            /* Multiple channels might have the same streamfile. Find the others
             * that are the same as this and clear them so they won't be closed
             * again. */
            for (j=0;j<vgmstream->channels;j++) {
                if (i!=j && vgmstream->ch[j].streamfile == 
                            vgmstream->ch[i].streamfile) {
                    vgmstream->ch[j].streamfile = NULL;
                }
            }
            vgmstream->ch[i].streamfile = NULL;
        }
    }

    if (vgmstream->loop_ch) free(vgmstream->loop_ch);
    if (vgmstream->start_ch) free(vgmstream->start_ch);
    if (vgmstream->ch) free(vgmstream->ch);
    /* the start_vgmstream is considered just data */
    if (vgmstream->start_vgmstream) free(vgmstream->start_vgmstream);

    free(vgmstream);
}

int32_t get_vgmstream_play_samples(double looptimes, double fadeseconds, double fadedelayseconds, VGMSTREAM * vgmstream) {
    if (vgmstream->loop_flag) {
        return vgmstream->loop_start_sample+(vgmstream->loop_end_sample-vgmstream->loop_start_sample)*looptimes+(fadedelayseconds+fadeseconds)*vgmstream->sample_rate;
    } else return vgmstream->num_samples;
}

void render_vgmstream(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    switch (vgmstream->layout_type) {
        case layout_interleave:
        case layout_interleave_shortblock:
            render_vgmstream_interleave(buffer,sample_count,vgmstream);
            break;
#ifdef VGM_USE_VORBIS
        case layout_ogg_vorbis:
#endif
#ifdef VGM_USE_MPEG
        case layout_fake_mpeg:
        case layout_mpeg:
#endif
        case layout_dtk_interleave:
        case layout_none:
            render_vgmstream_nolayout(buffer,sample_count,vgmstream);
            break;
        case layout_ast_blocked:
        case layout_halpst_blocked:
		case layout_xa_blocked:
		case layout_ea_blocked:
		case layout_eacs_blocked:
		case layout_caf_blocked:
        case layout_wsi_blocked:
        case layout_str_snds_blocked:
        case layout_ws_aud_blocked:
		case layout_matx_blocked:
        case layout_de2_blocked:
		case layout_vs_blocked:
		case layout_emff_ps2_blocked:
		case layout_emff_ngc_blocked:
		case layout_gsb_blocked:
		case layout_xvas_blocked:
		case layout_thp_blocked:
		case layout_filp_blocked:
            render_vgmstream_blocked(buffer,sample_count,vgmstream);
            break;
        case layout_interleave_byte:
            render_vgmstream_interleave_byte(buffer,sample_count,vgmstream);
            break;
        case layout_acm:
        case layout_mus_acm:
            render_vgmstream_mus_acm(buffer,sample_count,vgmstream);
            break;
        case layout_aix:
            render_vgmstream_aix(buffer,sample_count,vgmstream);
            break;
        case layout_aax:
            render_vgmstream_aax(buffer,sample_count,vgmstream);
            break;
    }
}

int get_vgmstream_samples_per_frame(VGMSTREAM * vgmstream) {
    switch (vgmstream->coding_type) {
        case coding_CRI_ADX:
        case coding_CRI_ADX_enc:
        case coding_L5_555:
            return 32;
        case coding_NGC_DSP:
            return 14;
        case coding_PCM16LE:
		case coding_PCM16LE_int:
        case coding_PCM16BE:
        case coding_PCM8:
		case coding_PCM8_int:
        case coding_PCM8_SB_int:
        case coding_PCM8_U_int:
#ifdef VGM_USE_VORBIS
        case coding_ogg_vorbis:
#endif
#ifdef VGM_USE_MPEG
        case coding_fake_MPEG2_L2:
        case coding_MPEG1_L1:
        case coding_MPEG1_L2:
        case coding_MPEG1_L3:
        case coding_MPEG2_L1:
        case coding_MPEG2_L2:
        case coding_MPEG2_L3:
        case coding_MPEG25_L1:
        case coding_MPEG25_L2:
        case coding_MPEG25_L3:
#endif
        case coding_SDX2:
        case coding_SDX2_int:
        case coding_ACM:
        case coding_NWA0:
        case coding_NWA1:
        case coding_NWA2:
        case coding_NWA3:
        case coding_NWA4:
        case coding_NWA5:
            return 1;
        case coding_NDS_IMA:
				return (vgmstream->interleave_block_size-4)*2;
        case coding_NGC_DTK:
            return 28;
        case coding_G721:
        case coding_DVI_IMA:
		case coding_EACS_IMA:
        case coding_IMA:
            return 1;
        case coding_INT_IMA:
		case coding_INT_DVI_IMA:
        case coding_AICA:
			return 2;
        case coding_NGC_AFC:
        case coding_FFXI:
            return 16;
        case coding_PSX:
        case coding_PSX_badflags:
        case coding_invert_PSX:
		case coding_XA:
            return 28;
		case coding_XBOX:
			return 64;
		case coding_EAXA:
			return 28;
		case coding_EA_ADPCM:
			return 14*vgmstream->channels;
        case coding_WS:
            /* only works if output sample size is 8 bit, which is always
               is for WS ADPCM */
            return vgmstream->ws_output_size;
        case coding_MSADPCM:
            return (vgmstream->interleave_block_size-(7-1)*vgmstream->channels)*2/vgmstream->channels;
        case coding_NDS_PROCYON:
            return 30;
        default:
            return 0;
    }
}

int get_vgmstream_samples_per_shortframe(VGMSTREAM * vgmstream) {
    switch (vgmstream->coding_type) {
        case coding_NDS_IMA:
            return (vgmstream->interleave_smallblock_size-4)*2;
        default:
            return get_vgmstream_samples_per_frame(vgmstream);
    }
}

int get_vgmstream_frame_size(VGMSTREAM * vgmstream) {
    switch (vgmstream->coding_type) {
        case coding_CRI_ADX:
        case coding_CRI_ADX_enc:
        case coding_L5_555:
            return 18;
        case coding_NGC_DSP:
            return 8;
        case coding_PCM16LE:
		case coding_PCM16LE_int:
        case coding_PCM16BE:
            return 2;
        case coding_PCM8:
		case coding_PCM8_int:
        case coding_PCM8_SB_int:
        case coding_PCM8_U_int:
        case coding_SDX2:
        case coding_SDX2_int:
        case coding_NWA0:
        case coding_NWA1:
        case coding_NWA2:
        case coding_NWA3:
        case coding_NWA4:
        case coding_NWA5:
            return 1;
        case coding_NDS_IMA:
            return vgmstream->interleave_block_size;
        case coding_NGC_DTK:
            return 32;
		case coding_EACS_IMA:
			return 1;
        case coding_DVI_IMA:
        case coding_IMA:
        case coding_G721:
            return 0;
        case coding_NGC_AFC:
        case coding_FFXI:
            return 9;
        case coding_PSX:
        case coding_PSX_badflags:
        case coding_invert_PSX:
        case coding_NDS_PROCYON:
            return 16;
		case coding_XA:
			return 14*vgmstream->channels;
		case coding_XBOX:
			return 36;
		case coding_EA_ADPCM:
			return 30;
		case coding_EAXA:
			return 1; // the frame is variant in size
        case coding_WS:
            return vgmstream->current_block_size;
        case coding_INT_IMA:
        case coding_INT_DVI_IMA:
        case coding_AICA:
			return 1; 
        case coding_MSADPCM:
            return vgmstream->interleave_block_size;
        default:
            return 0;
    }
}

int get_vgmstream_shortframe_size(VGMSTREAM * vgmstream) {
    switch (vgmstream->coding_type) {
        case coding_NDS_IMA:
            return vgmstream->interleave_smallblock_size;
        default:
            return get_vgmstream_frame_size(vgmstream);
    }
}

void decode_vgmstream_mem(VGMSTREAM * vgmstream, int samples_written, int samples_to_do, sample * buffer, uint8_t * data, int channel) {

    switch (vgmstream->coding_type) {
        case coding_NGC_DSP:
            decode_ngc_dsp_mem(&vgmstream->ch[channel],
                    buffer+samples_written*vgmstream->channels+channel,
                    vgmstream->channels,vgmstream->samples_into_block,
                    samples_to_do, data);
            break;
        default:
            break;
    }
}

void decode_vgmstream(VGMSTREAM * vgmstream, int samples_written, int samples_to_do, sample * buffer) {
    int chan;

    switch (vgmstream->coding_type) {
        case coding_CRI_ADX:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_adx(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }

            break;
        case coding_CRI_ADX_enc:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_adx_enc(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }

            break;
        case coding_NGC_DSP:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ngc_dsp(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM16LE:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm16LE(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM16LE_int:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm16LE_int(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM16BE:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm16BE(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM8:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm8(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM8_int:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm8_int(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM8_SB_int:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm8_sb_int(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PCM8_U_int:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_pcm8_unsigned_int(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_NDS_IMA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_nds_ima(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
		case coding_XBOX:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_xbox_ima(vgmstream,&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
        case coding_NGC_DTK:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ngc_dtk(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
        case coding_G721:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_g721(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_NGC_AFC:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ngc_afc(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PSX:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_psx(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_PSX_badflags:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_psx_badflags(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_invert_PSX:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_invert_psx(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_FFXI:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ffxi_adpcm(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
		case coding_XA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_xa(vgmstream,buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
		case coding_EAXA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_eaxa(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
		case coding_EA_ADPCM:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ea_adpcm(vgmstream,buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
#ifdef VGM_USE_VORBIS
        case coding_ogg_vorbis:
            decode_ogg_vorbis(vgmstream->codec_data,
                    buffer+samples_written*vgmstream->channels,samples_to_do,
                    vgmstream->channels);
            break;
#endif
        case coding_SDX2:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_sdx2(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_SDX2_int:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_sdx2_int(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
		case coding_DVI_IMA:
		case coding_INT_DVI_IMA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_dvi_ima(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
		case coding_EACS_IMA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_eacs_ima(vgmstream,buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do,chan);
            }
            break;
		case coding_IMA:
        case coding_INT_IMA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ima(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_WS:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_ws(vgmstream,chan,buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
#ifdef VGM_USE_MPEG
        case coding_fake_MPEG2_L2:
            decode_fake_mpeg2_l2(
                    &vgmstream->ch[0],
                    vgmstream->codec_data,
                    buffer+samples_written*vgmstream->channels,samples_to_do);
            break;
        case coding_MPEG1_L1:
        case coding_MPEG1_L2:
        case coding_MPEG1_L3:
        case coding_MPEG2_L1:
        case coding_MPEG2_L2:
        case coding_MPEG2_L3:
        case coding_MPEG25_L1:
        case coding_MPEG25_L2:
        case coding_MPEG25_L3:
            decode_mpeg(
                    &vgmstream->ch[0],
                    vgmstream->codec_data,
                    buffer+samples_written*vgmstream->channels,samples_to_do,
                    vgmstream->channels);
            break;
#endif
        case coding_ACM:
            /* handled in its own layout, here to quiet compiler */
            break;
        case coding_NWA0:
        case coding_NWA1:
        case coding_NWA2:
        case coding_NWA3:
        case coding_NWA4:
        case coding_NWA5:
            decode_nwa(((nwa_codec_data*)vgmstream->codec_data)->nwa,
                    buffer+samples_written*vgmstream->channels,
                    samples_to_do
                    );
            break;
        case coding_MSADPCM:
            if (vgmstream->channels == 2) {
                decode_msadpcm_stereo(vgmstream,
                        buffer+samples_written*vgmstream->channels,
                        vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
		case coding_AICA:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_aica(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_NDS_PROCYON:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_nds_procyon(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }
            break;
        case coding_L5_555:
            for (chan=0;chan<vgmstream->channels;chan++) {
                decode_l5_555(&vgmstream->ch[chan],buffer+samples_written*vgmstream->channels+chan,
                        vgmstream->channels,vgmstream->samples_into_block,
                        samples_to_do);
            }

            break;
    }
}

int vgmstream_samples_to_do(int samples_this_block, int samples_per_frame, VGMSTREAM * vgmstream) {
    int samples_to_do;
    int samples_left_this_block;

    samples_left_this_block = samples_this_block - vgmstream->samples_into_block;
    samples_to_do = samples_left_this_block;

    /* fun loopy crap */
    /* Why did I think this would be any simpler? */
    if (vgmstream->loop_flag) {
        /* are we going to hit the loop end during this block? */
        if (vgmstream->current_sample+samples_left_this_block > vgmstream->loop_end_sample) {
            /* only do to just before it */
            samples_to_do = vgmstream->loop_end_sample-vgmstream->current_sample;
        }

        /* are we going to hit the loop start during this block? */
        if (!vgmstream->hit_loop && vgmstream->current_sample+samples_left_this_block > vgmstream->loop_start_sample) {
            /* only do to just before it */
            samples_to_do = vgmstream->loop_start_sample-vgmstream->current_sample;
        }

    }

    /* if it's a framed encoding don't do more than one frame */
    if (samples_per_frame>1 && (vgmstream->samples_into_block%samples_per_frame)+samples_to_do>samples_per_frame) samples_to_do=samples_per_frame-(vgmstream->samples_into_block%samples_per_frame);

    return samples_to_do;
}

/* return 1 if we just looped */
int vgmstream_do_loop(VGMSTREAM * vgmstream) {
/*    if (vgmstream->loop_flag) {*/
        /* is this the loop end? */
        if (vgmstream->current_sample==vgmstream->loop_end_sample) {
            /* against everything I hold sacred, preserve adpcm
             * history through loop for certain types */
            if (vgmstream->meta_type == meta_DSP_STD ||
                    vgmstream->meta_type == meta_DSP_RS03 ||
                    vgmstream->meta_type == meta_DSP_CSTR || 
					vgmstream->coding_type == coding_PSX ||
                    vgmstream->coding_type == coding_invert_PSX ||
                    vgmstream->coding_type == coding_PSX_badflags) {
                int i;
                for (i=0;i<vgmstream->channels;i++) {
                    vgmstream->loop_ch[i].adpcm_history1_16 = vgmstream->ch[i].adpcm_history1_16;
                    vgmstream->loop_ch[i].adpcm_history2_16 = vgmstream->ch[i].adpcm_history2_16;
                    vgmstream->loop_ch[i].adpcm_history1_32 = vgmstream->ch[i].adpcm_history1_32;
                    vgmstream->loop_ch[i].adpcm_history2_32 = vgmstream->ch[i].adpcm_history2_32;
                }
            }
#ifdef DEBUG
            {
               int i;
               for (i=0;i<vgmstream->channels;i++) {
                   fprintf(stderr,"ch%d hist: %04x %04x loop hist: %04x %04x\n",i,
                           vgmstream->ch[i].adpcm_history1_16,vgmstream->ch[i].adpcm_history2_16,
                           vgmstream->loop_ch[i].adpcm_history1_16,vgmstream->loop_ch[i].adpcm_history2_16);
                   fprintf(stderr,"ch%d offset: %x loop offset: %x\n",i,
                           vgmstream->ch[i].offset,
                           vgmstream->loop_ch[i].offset);
               }
            }
#endif

#ifdef VGM_USE_VORBIS
            if (vgmstream->coding_type==coding_ogg_vorbis) {
                ogg_vorbis_codec_data *data =
                    (ogg_vorbis_codec_data *)(vgmstream->codec_data);
                OggVorbis_File *ogg_vorbis_file = &(data->ogg_vorbis_file);
                
                ov_pcm_seek_lap(ogg_vorbis_file, vgmstream->loop_sample);
            }
#endif
#ifdef VGM_USE_MPEG
            /* won't work for fake MPEG */
            if (vgmstream->layout_type==layout_mpeg) {
                off_t input_offset;
                mpeg_codec_data *data = vgmstream->codec_data;

                mpg123_feedseek(data->m,vgmstream->loop_sample,
                        SEEK_SET,&input_offset);
                vgmstream->loop_ch[0].offset =
                    vgmstream->loop_ch[0].channel_start_offset + input_offset;
                data->buffer_full = data->buffer_used = 0;
            }
#endif

            if (vgmstream->coding_type == coding_NWA0 ||
                    vgmstream->coding_type == coding_NWA1 ||
                    vgmstream->coding_type == coding_NWA2 ||
                    vgmstream->coding_type == coding_NWA3 ||
                    vgmstream->coding_type == coding_NWA4 ||
                    vgmstream->coding_type == coding_NWA5)
            {
                nwa_codec_data *data = vgmstream->codec_data;

                seek_nwa(data->nwa, vgmstream->loop_sample);
            }

            /* restore! */
            memcpy(vgmstream->ch,vgmstream->loop_ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);
            vgmstream->current_sample=vgmstream->loop_sample;
            vgmstream->samples_into_block=vgmstream->loop_samples_into_block;
            vgmstream->current_block_size=vgmstream->loop_block_size;
            vgmstream->current_block_offset=vgmstream->loop_block_offset;
            vgmstream->next_block_offset=vgmstream->loop_next_block_offset;

            return 1;
        }


        /* is this the loop start? */
        if (!vgmstream->hit_loop && vgmstream->current_sample==vgmstream->loop_start_sample) {
            /* save! */
            memcpy(vgmstream->loop_ch,vgmstream->ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);

            vgmstream->loop_sample=vgmstream->current_sample;
            vgmstream->loop_samples_into_block=vgmstream->samples_into_block;
            vgmstream->loop_block_size=vgmstream->current_block_size;
            vgmstream->loop_block_offset=vgmstream->current_block_offset;
            vgmstream->loop_next_block_offset=vgmstream->next_block_offset;
            vgmstream->hit_loop=1;
        }
        /*}*/
        return 0;
}

/* build a descriptive string */
void describe_vgmstream(VGMSTREAM * vgmstream, char * desc, int length) {
#define TEMPSIZE 256
    char temp[TEMPSIZE];

    if (!vgmstream) {
        snprintf(temp,TEMPSIZE,"NULL VGMSTREAM");
        concatn(length,desc,temp);
        return;
    }

    snprintf(temp,TEMPSIZE,"sample rate %d Hz\n"
            "channels: %d\n",
            vgmstream->sample_rate,vgmstream->channels);
    concatn(length,desc,temp);

    if (vgmstream->loop_flag) {
        snprintf(temp,TEMPSIZE,"loop start: %d samples (%.2lf seconds)\n"
                "loop end: %d samples (%.2lf seconds)\n",
                vgmstream->loop_start_sample,
                (double)vgmstream->loop_start_sample/vgmstream->sample_rate,
                vgmstream->loop_end_sample,
                (double)vgmstream->loop_end_sample/vgmstream->sample_rate);
        concatn(length,desc,temp);
    }

    snprintf(temp,TEMPSIZE,"stream total samples: %d (%.2lf seconds)\n",
            vgmstream->num_samples,
            (double)vgmstream->num_samples/vgmstream->sample_rate);
    concatn(length,desc,temp);

    snprintf(temp,TEMPSIZE,"encoding: ");
    concatn(length,desc,temp);

    switch (vgmstream->coding_type) {
        case coding_PCM16BE:
            snprintf(temp,TEMPSIZE,"Big Endian 16-bit PCM");
            break;
        case coding_PCM16LE:
            snprintf(temp,TEMPSIZE,"Little Endian 16-bit PCM");
            break;
        case coding_PCM16LE_int:
            snprintf(temp,TEMPSIZE,"Little Endian 16-bit PCM with 2 byte interleave");
            break;
        case coding_PCM8:
            snprintf(temp,TEMPSIZE,"8-bit PCM");
            break;
        case coding_PCM8_U_int:
            snprintf(temp,TEMPSIZE,"8-bit unsigned PCM with 1 byte interleave");
            break;
        case coding_PCM8_int:
            snprintf(temp,TEMPSIZE,"8-bit PCM with 1 byte interleave");
            break;
        case coding_PCM8_SB_int:
            snprintf(temp,TEMPSIZE,"8-bit PCM with sign bit, 1 byte interleave");
            break;
        case coding_NGC_DSP:
            snprintf(temp,TEMPSIZE,"Gamecube \"DSP\" 4-bit ADPCM");
            break;
        case coding_CRI_ADX:
            snprintf(temp,TEMPSIZE,"CRI ADX 4-bit ADPCM");
            break;
        case coding_CRI_ADX_enc:
            snprintf(temp,TEMPSIZE,"encrypted CRI ADX 4-bit ADPCM");
            break;
        case coding_NDS_IMA:
            snprintf(temp,TEMPSIZE,"NDS-style 4-bit IMA ADPCM");
            break;
        case coding_NGC_DTK:
            snprintf(temp,TEMPSIZE,"Gamecube \"ADP\"/\"DTK\" 4-bit ADPCM");
            break;
        case coding_G721:
            snprintf(temp,TEMPSIZE,"CCITT G.721 4-bit ADPCM");
            break;
        case coding_NGC_AFC:
            snprintf(temp,TEMPSIZE,"Gamecube \"AFC\" 4-bit ADPCM");
            break;
        case coding_PSX:
            snprintf(temp,TEMPSIZE,"Playstation 4-bit ADPCM");
            break;
        case coding_PSX_badflags:
            snprintf(temp,TEMPSIZE,"Playstation 4-bit ADPCM with bad flags");
            break;
        case coding_invert_PSX:
            snprintf(temp,TEMPSIZE,"BMDX \"encrypted\" Playstation 4-bit ADPCM");
            break;
        case coding_FFXI:
            snprintf(temp,TEMPSIZE,"FFXI Playstation-ish 4-bit ADPCM");
            break;
        case coding_XA:
            snprintf(temp,TEMPSIZE,"CD-ROM XA 4-bit ADPCM");
            break;
		case coding_XBOX:
            snprintf(temp,TEMPSIZE,"XBOX 4-bit IMA ADPCM");
            break;
		case coding_EAXA:
            snprintf(temp,TEMPSIZE,"Electronic Arts XA Based 4-bit ADPCM");
            break;
		case coding_EA_ADPCM:
            snprintf(temp,TEMPSIZE,"Electronic Arts XA Based (R1) 4-bit ADPCM");
            break;
#ifdef VGM_USE_VORBIS
        case coding_ogg_vorbis:
            snprintf(temp,TEMPSIZE,"Vorbis");
            break;
#endif
        case coding_SDX2:
            snprintf(temp,TEMPSIZE,"Squareroot-delta-exact (SDX2) 8-bit DPCM");
            break;
        case coding_SDX2_int:
            snprintf(temp,TEMPSIZE,"Squareroot-delta-exact (SDX2) 8-bit DPCM with 1 byte interleave");
            break;
        case coding_DVI_IMA:
            snprintf(temp,TEMPSIZE,"Intel DVI 4-bit IMA ADPCM");
            break;
        case coding_INT_DVI_IMA:
            snprintf(temp,TEMPSIZE,"Interleaved Intel DVI 4-bit IMA ADPCM");
            break;
		case coding_EACS_IMA:
            snprintf(temp,TEMPSIZE,"EACS 4-bit IMA ADPCM");
            break;
        case coding_INT_IMA:
            snprintf(temp,TEMPSIZE,"Interleaved 4-bit IMA ADPCM");
            break;
        case coding_IMA:
            snprintf(temp,TEMPSIZE,"4-bit IMA ADPCM");
            break;
        case coding_WS:
            snprintf(temp,TEMPSIZE,"Westwood Studios DPCM");
            break;
#ifdef VGM_USE_MPEG
        case coding_fake_MPEG2_L2:
            snprintf(temp,TEMPSIZE,"MPEG-2 Layer II Audio");
            break;
        case coding_MPEG1_L1:
            snprintf(temp,TEMPSIZE,"MPEG-1 Layer I Audio");
            break;
        case coding_MPEG1_L2:
            snprintf(temp,TEMPSIZE,"MPEG-1 Layer II Audio");
            break;
        case coding_MPEG1_L3:
            snprintf(temp,TEMPSIZE,"MPEG-1 Layer III Audio (MP3)");
            break;
        case coding_MPEG2_L1:
            snprintf(temp,TEMPSIZE,"MPEG-2 Layer I Audio");
            break;
        case coding_MPEG2_L2:
            snprintf(temp,TEMPSIZE,"MPEG-2 Layer II Audio");
            break;
        case coding_MPEG2_L3:
            snprintf(temp,TEMPSIZE,"MPEG-2 Layer III Audio (MP3)");
            break;
        case coding_MPEG25_L1:
            snprintf(temp,TEMPSIZE,"MPEG-2.5 Layer I Audio");
            break;
        case coding_MPEG25_L2:
            snprintf(temp,TEMPSIZE,"MPEG-2.5 Layer II Audio");
            break;
        case coding_MPEG25_L3:
            snprintf(temp,TEMPSIZE,"MPEG-2.5 Layer III Audio (MP3)");
            break;
#endif
        case coding_ACM:
            snprintf(temp,TEMPSIZE,"InterPlay ACM");
            break;
        case coding_NWA0:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 0");
            break;
        case coding_NWA1:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 1");
            break;
        case coding_NWA2:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 2");
            break;
        case coding_NWA3:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 3");
            break;
        case coding_NWA4:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 4");
            break;
        case coding_NWA5:
            snprintf(temp,TEMPSIZE,"NWA DPCM Level 5");
            break;
        case coding_MSADPCM:
            snprintf(temp,TEMPSIZE,"Microsoft 4-bit ADPCM");
            break;
        case coding_AICA:
            snprintf(temp,TEMPSIZE,"Yamaha AICA 4-bit ADPCM");
            break;
        case coding_NDS_PROCYON:
            snprintf(temp,TEMPSIZE,"Procyon Studio Digital Sound Elements NDS 4-bit APDCM");
            break;
        case coding_L5_555:
            snprintf(temp,TEMPSIZE,"Level-5 0x555 ADPCM");
            break;
        default:
            snprintf(temp,TEMPSIZE,"CANNOT DECODE");
    }
    concatn(length,desc,temp);

    snprintf(temp,TEMPSIZE,"\nlayout: ");
    concatn(length,desc,temp);

    switch (vgmstream->layout_type) {
        case layout_none:
            snprintf(temp,TEMPSIZE,"flat (no layout)");
            break;
        case layout_interleave:
            snprintf(temp,TEMPSIZE,"interleave");
            break;
        case layout_interleave_shortblock:
            snprintf(temp,TEMPSIZE,"interleave with short last block");
            break;
        case layout_interleave_byte:
            snprintf(temp,TEMPSIZE,"sub-frame interleave");
            break;
        case layout_dtk_interleave:
            snprintf(temp,TEMPSIZE,"ADP/DTK nibble interleave");
            break;
        case layout_ast_blocked:
            snprintf(temp,TEMPSIZE,"AST blocked");
            break;
        case layout_halpst_blocked:
            snprintf(temp,TEMPSIZE,"HALPST blocked");
            break;
		case layout_xa_blocked:
            snprintf(temp,TEMPSIZE,"CD-ROM XA");
            break;
		case layout_ea_blocked:
            snprintf(temp,TEMPSIZE,"Electronic Arts Audio Blocks");
            break;
		case layout_eacs_blocked:
            snprintf(temp,TEMPSIZE,"Electronic Arts (Old Version) Audio Blocks");
            break;
		case layout_caf_blocked:
            snprintf(temp,TEMPSIZE,"CAF blocked");
            break;
        case layout_wsi_blocked:
            snprintf(temp,TEMPSIZE,".wsi blocked");
            break;
        case layout_xvas_blocked:
            snprintf(temp,TEMPSIZE,".xvas blocked");
            break;
#ifdef VGM_USE_VORBIS
        case layout_ogg_vorbis:
            snprintf(temp,TEMPSIZE,"Ogg");
            break;
#endif
        case layout_str_snds_blocked:
            snprintf(temp,TEMPSIZE,".str SNDS blocked");
            break;
        case layout_ws_aud_blocked:
            snprintf(temp,TEMPSIZE,"Westwood Studios .aud blocked");
            break;
        case layout_matx_blocked:
            snprintf(temp,TEMPSIZE,"Matrix .matx blocked");
            break;
        case layout_de2_blocked:
            snprintf(temp,TEMPSIZE,"de2 blocked");
            break;
		case layout_vs_blocked:
            snprintf(temp,TEMPSIZE,"vs blocked");
            break;
        case layout_emff_ps2_blocked:
            snprintf(temp,TEMPSIZE,"EMFF (PS2) blocked");
            break;
        case layout_emff_ngc_blocked:
            snprintf(temp,TEMPSIZE,"EMFF (NGC/WII) blocked");
            break;
        case layout_gsb_blocked:
            snprintf(temp,TEMPSIZE,"GSB blocked");
            break;
        case layout_thp_blocked:
            snprintf(temp,TEMPSIZE,"THP Movie Audio blocked");
            break;
        case layout_filp_blocked:
            snprintf(temp,TEMPSIZE,"FILp blocked");
            break;
#ifdef VGM_USE_MPEG
        case layout_fake_mpeg:
            snprintf(temp,TEMPSIZE,"MPEG Audio stream with incorrect frame headers");
            break;
        case layout_mpeg:
            snprintf(temp,TEMPSIZE,"MPEG Audio stream");
            break;
#endif
        case layout_acm:
            snprintf(temp,TEMPSIZE,"ACM blocked");
            break;
        case layout_mus_acm:
            snprintf(temp,TEMPSIZE,"multiple ACM files, ACM blocked");
            break;
        case layout_aix:
            snprintf(temp,TEMPSIZE,"AIX interleave, internally 18-byte interleaved");
            break;
        case layout_aax:
            snprintf(temp,TEMPSIZE,"AAX blocked, 18-byte interleaved");
            break;
        default:
            snprintf(temp,TEMPSIZE,"INCONCEIVABLE");
    }
    concatn(length,desc,temp);

    snprintf(temp,TEMPSIZE,"\n");
    concatn(length,desc,temp);

    if (vgmstream->layout_type == layout_interleave || vgmstream->layout_type == layout_interleave_shortblock || vgmstream->layout_type == layout_interleave_byte) {
        snprintf(temp,TEMPSIZE,"interleave: %#x bytes\n",
                (int32_t)vgmstream->interleave_block_size);
        concatn(length,desc,temp);

        if (vgmstream->layout_type == layout_interleave_shortblock) {
            snprintf(temp,TEMPSIZE,"last block interleave: %#x bytes\n",
                    (int32_t)vgmstream->interleave_smallblock_size);
            concatn(length,desc,temp);
        }
    }

    snprintf(temp,TEMPSIZE,"metadata from: ");
    concatn(length,desc,temp);

    switch (vgmstream->meta_type) {
        case meta_RSTM:
            snprintf(temp,TEMPSIZE,"Nintendo RSTM header");
            break;
        case meta_STRM:
            snprintf(temp,TEMPSIZE,"Nintendo STRM header");
            break;
        case meta_ADX_03:
            snprintf(temp,TEMPSIZE,"CRI ADX header type 03");
            break;
        case meta_ADX_04:
            snprintf(temp,TEMPSIZE,"CRI ADX header type 04");
            break;
        case meta_ADX_05:
            snprintf(temp,TEMPSIZE,"CRI ADX header type 05");
            break;
        case meta_AIX:
            snprintf(temp,TEMPSIZE,"CRI AIX header");
            break;
        case meta_AAX:
            snprintf(temp,TEMPSIZE,"CRI AAX header");
            break;
        case meta_DSP_AGSC:
            snprintf(temp,TEMPSIZE,"Retro Studios AGSC header");
            break;
        case meta_NGC_ADPDTK:
            snprintf(temp,TEMPSIZE,"assumed Nintendo ADP by .adp extension and valid first frame");
            break;
        case meta_RSF:
            snprintf(temp,TEMPSIZE,"assumed Retro Studios RSF by .rsf extension and valid first bytes");
            break;
        case meta_AFC:
            snprintf(temp,TEMPSIZE,"Nintendo AFC header");
            break;
        case meta_AST:
            snprintf(temp,TEMPSIZE,"Nintendo AST header");
            break;
        case meta_HALPST:
            snprintf(temp,TEMPSIZE,"HAL Laboratory HALPST header");
            break;
        case meta_DSP_RS03:
            snprintf(temp,TEMPSIZE,"Retro Studios RS03 header");
            break;
        case meta_DSP_STD:
            snprintf(temp,TEMPSIZE,"Standard Nintendo DSP header");
            break;
        case meta_DSP_CSTR:
            snprintf(temp,TEMPSIZE,"Namco Cstr header");
            break;
        case meta_GCSW:
            snprintf(temp,TEMPSIZE,"GCSW header");
            break;
        case meta_PS2_SShd:
            snprintf(temp,TEMPSIZE,"SShd header");
            break;
		case meta_PS2_NPSF:
            snprintf(temp,TEMPSIZE,"Namco Production Sound File (NPSF) header");
            break;
        case meta_RWSD:
            snprintf(temp,TEMPSIZE,"Nintendo RWSD header (single stream)");
            break;
        case meta_RWAR:
            snprintf(temp,TEMPSIZE,"Nintendo RWAR header (single RWAV stream)");
            break;
        case meta_RWAV:
            snprintf(temp,TEMPSIZE,"Nintendo RWAV header");
            break;
        case meta_PSX_XA:
            snprintf(temp,TEMPSIZE,"RIFF/CDXA header");
            break;
		case meta_PS2_RXW:
            snprintf(temp,TEMPSIZE,"RXWS header)");
            break;
		case meta_PS2_RAW:
            snprintf(temp,TEMPSIZE,"assumed RAW Interleaved PCM by .int extension");
            break;
		case meta_PS2_OMU:
            snprintf(temp,TEMPSIZE,"Alter Echo OMU Header");
            break;
        case meta_DSP_STM:
            snprintf(temp,TEMPSIZE,"Nintendo STM header");
            break;
        case meta_PS2_EXST:
            snprintf(temp,TEMPSIZE,"EXST header");
            break;
        case meta_PS2_SVAG:
            snprintf(temp,TEMPSIZE,"Konami SVAG header");
            break;
        case meta_PS2_MIB:
            snprintf(temp,TEMPSIZE,"assumed MIB Interleaved file by .mib extension");
            break;
        case meta_PS2_MIB_MIH:
            snprintf(temp,TEMPSIZE,"assumed MIB with MIH Info Header file by .mib+.mih extension");
            break;
        case meta_DSP_MPDSP:
            snprintf(temp,TEMPSIZE,"Single DSP header stereo by .mpdsp extension");
            break;
        case meta_PS2_MIC:
            snprintf(temp,TEMPSIZE,"assume KOEI MIC file by .mic extension");
            break;
        case meta_DSP_JETTERS:
            snprintf(temp,TEMPSIZE,"Double DSP header stereo by _lr.dsp extension");
            break;
        case meta_DSP_MSS:
            snprintf(temp,TEMPSIZE,"Double DSP header stereo by .mss extension");
            break;
        case meta_DSP_GCM:
            snprintf(temp,TEMPSIZE,"Double DSP header stereo by .gcm extension");
            break;
		case meta_DSP_WII_IDSP:
            snprintf(temp,TEMPSIZE,"Wii IDSP Double DSP header");
            break;
        case meta_RSTM_SPM:
            snprintf(temp,TEMPSIZE,"Nintendo RSTM header and .brstmspm extension");
            break;
        case meta_RAW:
            snprintf(temp,TEMPSIZE,"assumed RAW PCM file by .raw extension");
            break;
		case meta_PS2_VAGi:
            snprintf(temp,TEMPSIZE,"Sony VAG Interleaved header (VAGi)");
            break;
		case meta_PS2_VAGp:
            snprintf(temp,TEMPSIZE,"Sony VAG Mono header (VAGp)");
            break;
		case meta_PS2_VAGs:
            snprintf(temp,TEMPSIZE,"Sony VAG Stereo header (VAGp)");
            break;
		case meta_PS2_VAGm:
            snprintf(temp,TEMPSIZE,"Sony VAG Mono header (VAGm)");
            break;
		case meta_PS2_pGAV:
            snprintf(temp,TEMPSIZE,"Sony VAG Stereo Little Endian header (pGAV)");
            break;
		case meta_PSX_GMS:
            snprintf(temp,TEMPSIZE,"assumed Grandia GMS file by .gms extension");
            break;
		case meta_PS2_STR:
            snprintf(temp,TEMPSIZE,"assumed STR + STH File by .str & .sth extension");
            break;
		case meta_PS2_ILD:
            snprintf(temp,TEMPSIZE,"ILD header");
            break;
		case meta_PS2_PNB:
            snprintf(temp,TEMPSIZE,"assumed PNB (PsychoNauts Bgm File) by .pnb extension");
            break;
		case meta_XBOX_WAVM:
            snprintf(temp,TEMPSIZE,"assumed Xbox WAVM file by .wavm extension");
            break;
		case meta_XBOX_RIFF:
            snprintf(temp,TEMPSIZE,"Xbox RIFF/WAVE file with 0x0069 Codec ID");
            break;
		case meta_DSP_STR:
            snprintf(temp,TEMPSIZE,"assumed Conan Gamecube STR File by .str extension");
            break;
		case meta_EAXA_R2:
            snprintf(temp,TEMPSIZE,"Electronic Arts XA R2");
            break;
		case meta_EAXA_R3:
            snprintf(temp,TEMPSIZE,"Electronic Arts XA R3");
            break;
		case meta_EA_ADPCM:
            snprintf(temp,TEMPSIZE,"Electronic Arts XA R1");
            break;
		case meta_EA_IMA:
            snprintf(temp,TEMPSIZE,"Electronic Arts container with IMA blocks");
            break;
		case meta_EAXA_PSX:
            snprintf(temp,TEMPSIZE,"Electronic Arts With PSX ADPCM");
            break;
		case meta_EA_PCM:
            snprintf(temp,TEMPSIZE,"Electronic Arts With PCM");
            break;
		case meta_CFN:
            snprintf(temp,TEMPSIZE,"Namco CAF Header");
            break;
		case meta_PS2_VPK:
            snprintf(temp,TEMPSIZE,"VPK Header");
            break;
        case meta_GENH:
            snprintf(temp,TEMPSIZE,"GENH Generic Header");
            break;
#ifdef VGM_USE_VORBIS
        case meta_ogg_vorbis:
            snprintf(temp,TEMPSIZE,"Ogg Vorbis");
            break;
        case meta_OGG_SLI:
            snprintf(temp,TEMPSIZE,"Ogg Vorbis with .sli (start,length) for looping");
            break;
        case meta_OGG_SLI2:
            snprintf(temp,TEMPSIZE,"Ogg Vorbis with .sli (from,to) for looping");
            break;
        case meta_OGG_SFL:
            snprintf(temp,TEMPSIZE,"Ogg Vorbis with SFPL for looping");
            break;
        case meta_um3_ogg:
            snprintf(temp,TEMPSIZE,"Ogg Vorbis, Ultramarine3 \"encryption\"");
            break;
#endif
        case meta_DSP_SADB:
            snprintf(temp,TEMPSIZE,"sadb header");
            break;
        case meta_SADL:
            snprintf(temp,TEMPSIZE,"sadl header");
            break;
        case meta_PS2_BMDX:
            snprintf(temp,TEMPSIZE,"Beatmania .bmdx header");
            break;
        case meta_DSP_WSI:
            snprintf(temp,TEMPSIZE,".wsi header");
            break;
        case meta_AIFC:
            snprintf(temp,TEMPSIZE,"Audio Interchange File Format AIFF-C");
            break;
        case meta_AIFF:
            snprintf(temp,TEMPSIZE,"Audio Interchange File Format");
            break;
        case meta_STR_SNDS:
            snprintf(temp,TEMPSIZE,".str SNDS SHDR chunk");
            break;
        case meta_WS_AUD:
            snprintf(temp,TEMPSIZE,"Westwood Studios .aud header");
            break;
        case meta_WS_AUD_old:
            snprintf(temp,TEMPSIZE,"Westwood Studios .aud (old) header");
            break;
#ifdef VGM_USE_MPEG
        case meta_AHX:
            snprintf(temp,TEMPSIZE,"CRI AHX header");
            break;
#endif
        case meta_PS2_IVB:
            snprintf(temp,TEMPSIZE,"IVB/BVII header");
            break;
        case meta_PS2_SVS:
            snprintf(temp,TEMPSIZE,"Square SVS header");
            break;
        case meta_RIFF_WAVE:
            snprintf(temp,TEMPSIZE,"RIFF WAVE header");
            break;
        case meta_RIFF_WAVE_POS:
            snprintf(temp,TEMPSIZE,"RIFF WAVE header and .pos for looping");
            break;
        case meta_NWA:
            snprintf(temp,TEMPSIZE,"Visual Art's NWA header");
            break;
        case meta_NWA_NWAINFOINI:
            snprintf(temp,TEMPSIZE,"Visual Art's NWA header and NWAINFO.INI for looping");
            break;
        case meta_NWA_GAMEEXEINI:
            snprintf(temp,TEMPSIZE,"Visual Art's NWA header and Gameexe.ini for looping");
            break;
        case meta_XSS:
            snprintf(temp,TEMPSIZE,"Dino Crisis 3 XSS File");
            break;
        case meta_HGC1:
            snprintf(temp,TEMPSIZE,"Knights of the Temple 2 hgC1 Header");
            break;
        case meta_AUS:
            snprintf(temp,TEMPSIZE,"Capcom AUS Header");
            break;
        case meta_RWS:
            snprintf(temp,TEMPSIZE,"RWS Header");
            break;
        case meta_EACS_PC:
            snprintf(temp,TEMPSIZE,"EACS Header (PC)");
            break;
        case meta_EACS_PSX:
            snprintf(temp,TEMPSIZE,"EACS Header (PSX)");
            break;
        case meta_EACS_SAT:
            snprintf(temp,TEMPSIZE,"EACS Header (SATURN)");
            break;
		case meta_SL3:
            snprintf(temp,TEMPSIZE,"SL3 Header");
            break;
		case meta_FSB1:
            snprintf(temp,TEMPSIZE,"FMOD Sample Bank (FSB1) Header");
            break;
		case meta_FSB3:
            snprintf(temp,TEMPSIZE,"FMOD Sample Bank (FSB3) Header");
            break;
		case meta_FSB4:
            snprintf(temp,TEMPSIZE,"FMOD Sample Bank (FSB4) Header");
            break;
		case meta_RWX:
            snprintf(temp,TEMPSIZE,"RWX Header");
            break;
		case meta_XWB:
            snprintf(temp,TEMPSIZE,"XWB WBND Header");
            break;
		case meta_XA30:
            snprintf(temp,TEMPSIZE,"XA30 Header");
            break;
		case meta_MUSC:
            snprintf(temp,TEMPSIZE,"MUSC Header");
            break;
		case meta_MUSX_V004:
            snprintf(temp,TEMPSIZE,"MUSX / Version 004 Header");
            break;
		case meta_MUSX_V006:
            snprintf(temp,TEMPSIZE,"MUSX / Version 006 Header");
            break;
		case meta_MUSX_V010:
            snprintf(temp,TEMPSIZE,"MUSX / Version 010 Header");
            break;
		case meta_MUSX_V201:
            snprintf(temp,TEMPSIZE,"MUSX / Version 201 Header");
            break;
		case meta_LEG:
            snprintf(temp,TEMPSIZE,"Legaia 2 - Duel Saga LEG Header");
            break;
		case meta_FILP:
            snprintf(temp,TEMPSIZE,"Bio Hazard - Gun Survivor FILp Header");
            break;
		case meta_IKM:
            snprintf(temp,TEMPSIZE,"Zwei!! IKM Header");
            break;
		case meta_SFS:
            snprintf(temp,TEMPSIZE,"Baroque SFS Header");
            break;
		case meta_DVI:
            snprintf(temp,TEMPSIZE,"DVI Header");
            break;
		case meta_KCEY:
            snprintf(temp,TEMPSIZE,"KCEYCOMP Header");
            break;
		case meta_BG00:
            snprintf(temp,TEMPSIZE,"Falcom BG00 Header");
            break;
		case meta_PS2_RSTM:
            snprintf(temp,TEMPSIZE,"Rockstar Games RSTM Header");
            break;
        case meta_ACM:
            snprintf(temp,TEMPSIZE,"InterPlay ACM Header");
            break;
        case meta_MUS_ACM:
            snprintf(temp,TEMPSIZE,"MUS playlist and multiple InterPlay ACM Headered files");
            break;
		case meta_PS2_KCES:
            snprintf(temp,TEMPSIZE,"Konami KCES Header");
            break;
        case meta_PS2_DXH:
            snprintf(temp,TEMPSIZE,"Tokobot Plus DXH Header");
            break;
        case meta_PS2_PSH:
            snprintf(temp,TEMPSIZE,"Dawn of Mana - Seiken Densetsu 4 PSH Header");
            break;
        case meta_RIFF_WAVE_labl_Marker:
            snprintf(temp,TEMPSIZE,"RIFF WAVE header with loop markers");
            break;
        case meta_RIFF_WAVE_smpl:
            snprintf(temp,TEMPSIZE,"RIFF WAVE header with sample looping info");
            break;
		case meta_PCM:
            snprintf(temp,TEMPSIZE,"PCM file with custom header");
            break;
		case meta_PS2_RKV:
            snprintf(temp,TEMPSIZE,"Legacy of Kain - Blood Omen 2 RKV Header");
            break;
		case meta_PS2_PSW:
            snprintf(temp,TEMPSIZE,"Rayman Raving Rabbids Riff Container File");
            break;
		case meta_PS2_VAS:
            snprintf(temp,TEMPSIZE,"Pro Baseball Spirits 5 VAS Header");
            break;
		case meta_PS2_TEC:
            snprintf(temp,TEMPSIZE,"assumed TECMO badflagged stream by .tec extension");
            break;
		case meta_XBOX_WVS:
            snprintf(temp,TEMPSIZE,"Metal Arms WVS Header");
            break;
		case meta_XBOX_STMA:
			snprintf(temp,TEMPSIZE,"Midnight Club 2 STMA Header");
			break;
		case meta_XBOX_MATX:
			snprintf(temp,TEMPSIZE,"assumed Matrix file by .matx extension");
			break;
        case meta_DE2:
            snprintf(temp,TEMPSIZE,"gurumin .de2 with embedded funky RIFF");
            break;
		case meta_VS:
            snprintf(temp,TEMPSIZE,"Men in Black VS Header");
            break;
        case meta_DC_STR:
            snprintf(temp,TEMPSIZE,"Sega Stream Asset Builder header");
            break;
        case meta_DC_STR_V2:
            snprintf(temp,TEMPSIZE,"variant of Sega Stream Asset Builder header");
            break;
        case meta_XBOX_XMU:
            snprintf(temp,TEMPSIZE,"XMU header");
            break;
        case meta_XBOX_XVAS:
            snprintf(temp,TEMPSIZE,"assumed TMNT file by .xvas extension");
            break;
		case meta_PS2_XA2:
            snprintf(temp,TEMPSIZE,"assumed XA2 file by .xa2 extension");
            break;
		case meta_DC_IDVI:
            snprintf(temp,TEMPSIZE,"IDVI Header");
            break;
		case meta_NGC_YMF:
            snprintf(temp,TEMPSIZE,"YMF DSP Header");
            break;
		case meta_PS2_CCC:
            snprintf(temp,TEMPSIZE,"CCC Header");
            break;
		case meta_PSX_FAG:
            snprintf(temp,TEMPSIZE,"FAG Header");
            break;
		case meta_PS2_MIHB:
            snprintf(temp,TEMPSIZE,"Merged MIH+MIB");
            break;
        case meta_DSP_WII_MUS:
            snprintf(temp,TEMPSIZE,"mus header");
            break;
        case meta_WII_SNG:
            snprintf(temp,TEMPSIZE,"SNG DSP Header");
            break;
		case meta_RSD2VAG:
            snprintf(temp,TEMPSIZE,"RSD2/VAG Header");
            break;
		case meta_RSD2PCMB:
            snprintf(temp,TEMPSIZE,"RSD2/PCMB Header");
            break;
		case meta_RSD2XADP:
            snprintf(temp,TEMPSIZE,"RSD2/XADP Header");
            break;
		case meta_RSD3PCM:
            snprintf(temp,TEMPSIZE,"RSD3/PCM Header");
            break;
		case meta_RSD4PCMB:
            snprintf(temp,TEMPSIZE,"RSD4/PCMB Header");
            break;
		case meta_RSD4PCM:
            snprintf(temp,TEMPSIZE,"RSD4/PCM Header");
            break;
		case meta_RSD4VAG:
            snprintf(temp,TEMPSIZE,"RSD4/VAG Header");
            break;
		case meta_RSD6XADP:
            snprintf(temp,TEMPSIZE,"RSD6/XADP Header");
            break;
		case meta_RSD6VAG:
            snprintf(temp,TEMPSIZE,"RSD6/VAG Header");
            break;
        case meta_RSD6WADP:
            snprintf(temp,TEMPSIZE,"RSD6/WADP Header");
            break;
		case meta_DC_ASD:
            snprintf(temp,TEMPSIZE,"ASD Header");
            break;
		case meta_NAOMI_SPSD:
            snprintf(temp,TEMPSIZE,"SPSD Header");
            break;
        case meta_FFXI_BGW:
            snprintf(temp,TEMPSIZE,"BGW BGMStream header");
            break;
        case meta_FFXI_SPW:
            snprintf(temp,TEMPSIZE,"SPW SeWave header");
            break;
        case meta_PS2_ASS:
            snprintf(temp,TEMPSIZE,"ASS Header");
            break;
		case meta_IDSP:
            snprintf(temp,TEMPSIZE,"IDSP Header");
            break;
		case meta_IDSP2:
            snprintf(temp,TEMPSIZE,"IDSP Header");
            break;
		case meta_WAA_WAC_WAD_WAM:
            snprintf(temp,TEMPSIZE,"WAA/WAC/WAD/WAM RIFF Header");
            break;
		case meta_PS2_SEG:
            snprintf(temp,TEMPSIZE,"SEG (PS2) Header");
            break;
		case meta_NDS_STRM_FFTA2:
            snprintf(temp,TEMPSIZE,"Final Fantasy Tactics A2 RIFF Header");
            break;
		case meta_STR_ASR:
            snprintf(temp,TEMPSIZE,"Donkey Kong Jet Race KNON/WII Header");
            break;
		case meta_ZWDSP:
            snprintf(temp,TEMPSIZE,"Zack and Wiki custom DSP Header");
            break;
		case meta_GCA:
            snprintf(temp,TEMPSIZE,"GCA DSP Header");
            break;
		case meta_SPT_SPD:
            snprintf(temp,TEMPSIZE,"SPT+SPD DSP Header");
            break;
		case meta_ISH_ISD:
            snprintf(temp,TEMPSIZE,"ISH+ISD DSP Header");
            break;
		case meta_YDSP:
            snprintf(temp,TEMPSIZE,"YDSP Header");
            break;
		case meta_MSVP:
            snprintf(temp,TEMPSIZE,"MSVP Header");
            break;
		case meta_NGC_SSM:
            snprintf(temp,TEMPSIZE,"SSM DSP Header");
            break;
		case meta_PS2_JOE:
            snprintf(temp,TEMPSIZE,"Disney/Pixar JOE Header");
            break;
		case meta_VGS:
            snprintf(temp,TEMPSIZE,"Guitar Hero Encore Rocks the 80's Header");
            break;
		case meta_DC_WAV_DCS:
            snprintf(temp,TEMPSIZE,"Evil Twin WAV+DCS Header");
            break;
		case meta_WII_SMP:
            snprintf(temp,TEMPSIZE,"SMP DSP Header");
            break;
		case meta_EMFF_PS2:
		case meta_EMFF_NGC:
            snprintf(temp,TEMPSIZE,"Eidos Music File Format Header");
            break;
		case meta_THP:
            snprintf(temp,TEMPSIZE,"THP Movie File Format Header");
            break;
		case meta_STS_WII:
            snprintf(temp,TEMPSIZE,"Shikigami no Shiro (WII) Header");
            break;
		case meta_PS2_P2BT:
            snprintf(temp,TEMPSIZE,"Pop'n'Music 7 Header");
            break;
		case meta_PS2_GBTS:
            snprintf(temp,TEMPSIZE,"Pop'n'Music 9 Header");
            break;
		case meta_NGC_IADP:
            snprintf(temp,TEMPSIZE,"Dr MUTO Header");
            break;
        case meta_RSTM_shrunken:
            snprintf(temp,TEMPSIZE,"Nintendo RSTM header, corrupted by Atlus");
            break;
        case meta_RIFF_WAVE_MWV:
            snprintf(temp,TEMPSIZE,"RIFF WAVE header with .mwv flavoring");
            break;
        case meta_FFCC_STR:
            snprintf(temp,TEMPSIZE,"Final Fantasy: Crystal Chronicles STR header");
            break;
        default:
            snprintf(temp,TEMPSIZE,"THEY SHOULD HAVE SENT A POET");
    }
    concatn(length,desc,temp);
}

/* */
const char * const dfs_pairs[][2] = {
    {"L","R"},
    {"l","r"},
    {"_0","_1"},
    {"left","right"},
    {"Left","Right"},
};
#define DFS_PAIR_COUNT (sizeof(dfs_pairs)/sizeof(dfs_pairs[0]))

void try_dual_file_stereo(VGMSTREAM * opened_stream, STREAMFILE *streamFile) {
    char filename[260];
    char filename2[260];
    char * ext;
    int dfs_name= -1; /*-1=no stereo, 0=opened_stream is left, 1=opened_stream is right */
    VGMSTREAM * new_stream = NULL;
    STREAMFILE *dual_stream = NULL;
    int i,j;

    if (opened_stream->channels != 1) return;
    
    streamFile->get_name(streamFile,filename,sizeof(filename));

	/* vgmstream's layout stuff currently assumes a single file */
	// fastelbja : no need ... this one works ok with dual file
    //if (opened_stream->layout != layout_none) return;

    /* we need at least a base and a name ending to replace */
    if (strlen(filename)<2) return;

    strcpy(filename2,filename);

    /* look relative to the extension; */
    ext = (char *)filename_extension(filename2);

    /* we treat the . as part of the extension */
    if (ext-filename2 >= 1 && ext[-1]=='.') ext--;

    for (i=0; dfs_name==-1 && i<DFS_PAIR_COUNT; i++) {
        for (j=0; dfs_name==-1 && j<2; j++) {
            /* find a postfix on the name */
            if (!memcmp(ext-strlen(dfs_pairs[i][j]),
                        dfs_pairs[i][j],
                        strlen(dfs_pairs[i][j]))) {
                int other_name=j^1;
                int moveby;
                dfs_name=j;

                /* move the extension */
                moveby = strlen(dfs_pairs[i][other_name]) -
                    strlen(dfs_pairs[i][dfs_name]);
                memmove(ext+moveby,ext,strlen(ext)+1); /* terminator, too */

                /* make the new name */
                memcpy(ext+moveby-strlen(dfs_pairs[i][other_name]),dfs_pairs[i][other_name],strlen(dfs_pairs[i][other_name]));
            }
        }
    }

    /* did we find a name for the other file? */
    if (dfs_name==-1) goto fail;

#if 0
    printf("input is:            %s\n"
            "other file would be: %s\n",
            filename,filename2);
#endif

    dual_stream = streamFile->open(streamFile,filename2,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!dual_stream) goto fail;

    new_stream = init_vgmstream_internal(dual_stream,
            0   /* don't do dual file on this, to prevent recursion */
            );
    close_streamfile(dual_stream);

    /* see if we were able to open the file, and if everything matched nicely */
    if (new_stream &&
            new_stream->channels == 1 &&
            /* we have seen legitimate pairs where these are off by one... */
            /* but leaving it commented out until I can find those and recheck */
            /* abs(new_stream->num_samples-opened_stream->num_samples <= 1) && */
            new_stream->num_samples == opened_stream->num_samples &&
            new_stream->sample_rate == opened_stream->sample_rate &&
            new_stream->meta_type == opened_stream->meta_type &&
            new_stream->coding_type == opened_stream->coding_type &&
            new_stream->layout_type == opened_stream->layout_type &&
            new_stream->loop_flag == opened_stream->loop_flag &&
            /* check these even if there is no loop, because they should then
             * be zero in both */
            new_stream->loop_start_sample == opened_stream->loop_start_sample &&
            new_stream->loop_end_sample == opened_stream->loop_end_sample &&
            /* check even if the layout doesn't use them, because it is
             * difficult to determine when it does, and they should be zero
             * otherwise, anyway */
            new_stream->interleave_block_size == opened_stream->interleave_block_size &&
            new_stream->interleave_smallblock_size == opened_stream->interleave_smallblock_size) {
        /* We seem to have a usable, matching file. Merge in the second channel. */
        VGMSTREAMCHANNEL * new_chans;
        VGMSTREAMCHANNEL * new_loop_chans = NULL;
        VGMSTREAMCHANNEL * new_start_chans = NULL;

        /* build the channels */
        new_chans = calloc(2,sizeof(VGMSTREAMCHANNEL));
        if (!new_chans) goto fail;

        memcpy(&new_chans[dfs_name],&opened_stream->ch[0],sizeof(VGMSTREAMCHANNEL));
        memcpy(&new_chans[dfs_name^1],&new_stream->ch[0],sizeof(VGMSTREAMCHANNEL));

        /* loop and start will be initialized later, we just need to
         * allocate them here */
        new_start_chans = calloc(2,sizeof(VGMSTREAMCHANNEL));
        if (!new_start_chans) {
            free(new_chans);
            goto fail;
        }

        if (opened_stream->loop_ch) {
            new_loop_chans = calloc(2,sizeof(VGMSTREAMCHANNEL));
            if (!new_loop_chans) {
                free(new_chans);
                free(new_start_chans);
                goto fail;
            }
        }

        /* remove the existing structures */
        /* not using close_vgmstream as that would close the file */
        free(opened_stream->ch);
        free(new_stream->ch);

        free(opened_stream->start_ch);
        free(new_stream->start_ch);

        if (opened_stream->loop_ch) {
            free(opened_stream->loop_ch);
            free(new_stream->loop_ch);
        }

        /* fill in the new structures */
        opened_stream->ch = new_chans;
        opened_stream->start_ch = new_start_chans;
        opened_stream->loop_ch = new_loop_chans;

        /* stereo! */
        opened_stream->channels = 2;

        /* discard the second VGMSTREAM */
        free(new_stream);
    }
fail:
    return;
}
