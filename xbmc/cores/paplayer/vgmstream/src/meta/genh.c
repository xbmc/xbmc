#include "../vgmstream.h"
#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"
#ifdef VGM_USE_MPEG
#include <mpg123.h>
#endif

/* GENH is an artificial "generic" header for headerless streams */

VGMSTREAM * init_vgmstream_genh(STREAMFILE *streamFile) {
    
    VGMSTREAM * vgmstream = NULL;
    
    int32_t channel_count;
    int32_t interleave;
    int32_t sample_rate;
    int32_t loop_start;
    int32_t loop_end;
    int32_t start_offset;
    int32_t header_size;
    int32_t coef[2];
    int32_t coef_splitted[2];
    int32_t dsp_interleave_type;
    int32_t coef_type;

    char filename[260];
    int coding;
#ifdef VGM_USE_MPEG
    mpeg_codec_data *data = NULL;
#endif

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("genh",filename_extension(filename))) goto fail;

    /* check header magic */
    if (read_32bitBE(0x0,streamFile) != 0x47454e48) goto fail;

    /* check channel count (needed for ADP/DTK check) */
    channel_count = read_32bitLE(0x4,streamFile);
    if (channel_count < 1) goto fail;

    /* check format */
    /* 0 = PSX ADPCM */
    /* 1 = XBOX IMA ADPCM */
    /* 2 = NGC ADP/DTK ADPCM */
    /* 3 = 16bit big endian PCM */
    /* 4 = 16bit little endian PCM */
    /* 5 = 8bit PCM */
    /* 6 = SDX2 */
    /* 7 = DVI IMA */
    /* 8 = MPEG-1 Layer III, possibly also the MPEG-2 and 2.5 extensions */
    /* 9 = IMA */
    /* 10 = AICA ADPCM */
    /* 11 = MS ADPCM */
    /* 12 = NGC DSP */
    /* 13 = 8bit unsingned PCM */
    /* 14 = PSX ADPCM (bad flagged) */
    /* ... others to come */
    switch (read_32bitLE(0x18,streamFile)) {
        case 0:
            coding = coding_PSX;
            break;
        case 1:
            coding = coding_XBOX;
            break;
        case 2:
            coding = coding_NGC_DTK;
            if (channel_count != 2) goto fail;
            break;
        case 3:
            coding = coding_PCM16BE;
            break;
        case 4:
            coding = coding_PCM16LE;
            break;
        case 5:
            coding = coding_PCM8;
            break;
        case 6:
            coding = coding_SDX2;
            break;
        case 7:
            coding = coding_DVI_IMA;
            break;
#ifdef VGM_USE_MPEG
        case 8:
            /* we say MPEG-1 L3 here, but later find out exactly which */
            coding = coding_MPEG1_L3;
            break;
#endif
        case 9:
            coding = coding_IMA;
            break;
        case 10:
            coding = coding_AICA;
            break;
        case 11:
            coding = coding_MSADPCM;
            break;
        case 12:
            coding = coding_NGC_DSP;
            break;
        case 13:
            coding = coding_PCM8_U_int;
            break;
        case 14:
            coding = coding_PSX_badflags;
            break;
        default:
            goto fail;
    }

    start_offset = read_32bitLE(0x1C,streamFile);
    header_size = read_32bitLE(0x20,streamFile);

    /* HACK to support old genh */
    if (header_size == 0) {
        start_offset = 0x800;
        header_size = 0x800;
    }

    /* check for audio data start past header end */
    if (header_size > start_offset) goto fail;

    interleave = read_32bitLE(0x8,streamFile);
    sample_rate = read_32bitLE(0xc,streamFile);
    loop_start = read_32bitLE(0x10,streamFile);
    loop_end = read_32bitLE(0x14,streamFile);
    
    coef[0] = read_32bitLE(0x24,streamFile);
    coef[1] = read_32bitLE(0x28,streamFile);
    dsp_interleave_type = read_32bitLE(0x2C,streamFile);
    coef_type = read_32bitLE(0x30,streamFile); /*	0 - normal coefs
                                                    1 - splitted coefs (16byte rows)  */
    coef_splitted[0] = read_32bitLE(0x34,streamFile);
    coef_splitted[1] = read_32bitLE(0x38,streamFile);
    //if (coding == coding_XBOX && channel_count != 2) goto fail;

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,(loop_start!=-1));
    if (!vgmstream) goto fail;

    /* fill in the vital information */

    vgmstream->channels = channel_count;
    vgmstream->sample_rate = sample_rate;
    vgmstream->num_samples = loop_end;
    vgmstream->loop_start_sample = loop_start;
    vgmstream->loop_end_sample = loop_end;
    vgmstream->loop_flag = (loop_start != -1);

    switch (coding) {
        case coding_PCM8_U_int:
            vgmstream->layout_type=layout_none;
            break;
        case coding_PCM16LE:
        case coding_PCM16BE:
        case coding_PCM8:
        case coding_SDX2:
        case coding_PSX:
        case coding_PSX_badflags:
        case coding_DVI_IMA:
        case coding_IMA:
        case coding_AICA:
            vgmstream->interleave_block_size = interleave;
            if (channel_count > 1)
            {
                if (coding == coding_SDX2) {
                    coding = coding_SDX2_int;
                    vgmstream->coding_type = coding_SDX2_int;
                }
                if(vgmstream->interleave_block_size==0xffffffff)
                    vgmstream->layout_type=layout_none;
                else {
                    vgmstream->layout_type = layout_interleave;
                    if(coding==coding_DVI_IMA)
                        coding=coding_INT_DVI_IMA;
                    if(coding==coding_IMA)
                        coding=coding_INT_IMA;
                }
            } else {
                vgmstream->layout_type = layout_none;
            }
            break;
        case coding_MSADPCM:
            if (channel_count != 2) goto fail;
            vgmstream->interleave_block_size = interleave;
            vgmstream->layout_type = layout_none;
            break;
        case coding_XBOX:
            vgmstream->layout_type = layout_none;
            break;
        case coding_NGC_DTK:
            vgmstream->layout_type = layout_dtk_interleave;
            break;
        case coding_NGC_DSP:
            if (dsp_interleave_type == 0) {
                vgmstream->layout_type = layout_interleave;
                vgmstream->interleave_block_size = interleave;
            } else if (dsp_interleave_type == 1) {
                vgmstream->layout_type = layout_interleave_byte;
                vgmstream->interleave_block_size = interleave;
            } else if (dsp_interleave_type == 2) {
                vgmstream->layout_type = layout_none;
            }
            break;
            
#ifdef VGM_USE_MPEG
        case coding_MPEG1_L3:
            vgmstream->layout_type = layout_mpeg;
            break;
#endif
    }
    
    vgmstream->coding_type = coding;
    vgmstream->meta_type = meta_GENH;
    
    /* open the file for reading by each channel */
    {
        int i;
        int j;

        STREAMFILE * chstreamfile = NULL;

        for (i=0;i<channel_count;i++) {
            off_t chstart_offset = start_offset;

            switch (coding) {
                case coding_PSX:
                case coding_PSX_badflags:
                case coding_PCM16BE:
                case coding_PCM16LE:
                case coding_SDX2:
                case coding_SDX2_int:
                case coding_DVI_IMA:
                case coding_IMA:
                case coding_PCM8:
                case coding_PCM8_U_int:
                case coding_AICA:
                case coding_INT_DVI_IMA:
                case coding_INT_IMA:
                    if (vgmstream->layout_type == layout_interleave) {
                        if (interleave >= 512) {
                            chstreamfile =
                                streamFile->open(streamFile,filename,interleave);
                        } else {
                            if (!chstreamfile)
                                chstreamfile =
                                    streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
                        }
                        chstart_offset =
                            start_offset+vgmstream->interleave_block_size*i;
                    } else {
                        chstreamfile =
                            streamFile->open(streamFile,filename,
                                    STREAMFILE_DEFAULT_BUFFER_SIZE);
                    }
                    break;
                case coding_XBOX:
                case coding_MSADPCM:
                    /* xbox's "interleave" is a lie, all channels start at same
                     * offset */
                    chstreamfile =
                        streamFile->open(streamFile,filename,
                                STREAMFILE_DEFAULT_BUFFER_SIZE);
                    break;
                case coding_NGC_DTK:
                    if (!chstreamfile) 
                        chstreamfile =
                            streamFile->open(streamFile,filename,32*0x400);
                    break;
                case coding_NGC_DSP:
                    if (!chstreamfile) 
                        chstreamfile =
                            streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

                    if (coef_type == 0) {
                        for (j=0;j<16;j++) {
                            vgmstream->ch[i].adpcm_coef[j] = read_16bitBE(coef[i]+j*2,streamFile);
                        }
                    } else if (coef_type == 1) {
                        for (j=0;j<8;j++) {
                            vgmstream->ch[i].adpcm_coef[j*2]=read_16bitBE(coef[i]+j*2,streamFile);
                            vgmstream->ch[i].adpcm_coef[j*2+1]=read_16bitBE(coef_splitted[i]+j*2,streamFile);
                        }
                    }
                    chstart_offset =start_offset+vgmstream->interleave_block_size*i;
                    break;

#ifdef VGM_USE_MPEG
                case coding_MPEG1_L3:
                    if (!chstreamfile)
                        chstreamfile =
                            streamFile->open(streamFile,filename,MPEG_BUFFER_SIZE);
                    break;
#endif
            }

            if (!chstreamfile) goto fail;

            vgmstream->ch[i].streamfile = chstreamfile;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=chstart_offset;
        }
    }

#ifdef VGM_USE_MPEG
    if (coding == coding_MPEG1_L3) {
        int rc;
        off_t read_offset;
        data = calloc(1,sizeof(mpeg_codec_data));
        if (!data) goto mpeg_fail;

        data->m = mpg123_new(NULL,&rc);
        if (rc==MPG123_NOT_INITIALIZED) {
            if (mpg123_init()!=MPG123_OK) goto mpeg_fail;
            data->m = mpg123_new(NULL,&rc);
            if (rc!=MPG123_OK) goto mpeg_fail;
        } else if (rc!=MPG123_OK) {
            goto mpeg_fail;
        }

        mpg123_param(data->m,MPG123_REMOVE_FLAGS,MPG123_GAPLESS,0.0);

        if (mpg123_open_feed(data->m)!=MPG123_OK) {
            goto mpeg_fail;
        }

        /* check format */
        read_offset=0;
        do {
            size_t bytes_done;
            if (read_streamfile(data->buffer, start_offset+read_offset,
                    MPEG_BUFFER_SIZE,vgmstream->ch[0].streamfile) !=
                    MPEG_BUFFER_SIZE) goto mpeg_fail;
            read_offset+=1;
            rc = mpg123_decode(data->m,data->buffer,MPEG_BUFFER_SIZE,
                    NULL,0,&bytes_done);
            if (rc != MPG123_OK && rc != MPG123_NEW_FORMAT &&
                    rc != MPG123_NEED_MORE) goto mpeg_fail;
        } while (rc != MPG123_NEW_FORMAT);

        {
            long rate;
            int channels,encoding;
            struct mpg123_frameinfo mi;
            rc = mpg123_getformat(data->m,&rate,&channels,&encoding);
            if (rc != MPG123_OK) goto mpeg_fail;
            if (rate != vgmstream->sample_rate ||
                    channels != vgmstream->channels ||
                    encoding != MPG123_ENC_SIGNED_16) goto mpeg_fail;
            mpg123_info(data->m,&mi);
            if (mi.rate != vgmstream->sample_rate) goto mpeg_fail;
            if (mi.version == MPG123_1_0 && mi.layer == 1)
                vgmstream->coding_type = coding_MPEG1_L1;
            else if (mi.version == MPG123_1_0 && mi.layer == 2)
                vgmstream->coding_type = coding_MPEG1_L2;
            else if (mi.version == MPG123_1_0 && mi.layer == 3)
                vgmstream->coding_type = coding_MPEG1_L3;
            else if (mi.version == MPG123_2_0 && mi.layer == 1)
                vgmstream->coding_type = coding_MPEG2_L1;
            else if (mi.version == MPG123_2_0 && mi.layer == 2)
                vgmstream->coding_type = coding_MPEG2_L2;
            else if (mi.version == MPG123_2_0 && mi.layer == 3)
                vgmstream->coding_type = coding_MPEG2_L3;
            else if (mi.version == MPG123_2_5 && mi.layer == 1)
                vgmstream->coding_type = coding_MPEG25_L1;
            else if (mi.version == MPG123_2_5 && mi.layer == 2)
                vgmstream->coding_type = coding_MPEG25_L2;
            else if (mi.version == MPG123_2_5 && mi.layer == 3)
                vgmstream->coding_type = coding_MPEG25_L3;
            else goto mpeg_fail;
        }

        /* reinit, to ignore the reading we've done so far */
        mpg123_open_feed(data->m);

        vgmstream->codec_data = data;
    }
#endif

    return vgmstream;

    /* clean up anything we may have opened */
#ifdef VGM_USE_MPEG
mpeg_fail:
    if (data) {
        mpg123_delete(data->m);
        free(data);
    }
#endif
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
