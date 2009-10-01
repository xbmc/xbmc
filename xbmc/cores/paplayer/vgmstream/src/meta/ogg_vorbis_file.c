#include "../vgmstream.h"

#ifdef VGM_USE_VORBIS

#include <stdio.h>
#include <string.h>
#include "meta.h"
#include "../util.h"
#include <vorbis/vorbisfile.h>


#define DEFAULT_BITSTREAM 0

static size_t read_func(void *ptr, size_t size, size_t nmemb, void * datasource)
{
    ogg_vorbis_streamfile * const ov_streamfile = datasource;
    size_t items_read;

    size_t bytes_read;
   
    bytes_read = read_streamfile(ptr, ov_streamfile->offset, size * nmemb,
            ov_streamfile->streamfile);

    items_read = bytes_read / size;

    ov_streamfile->offset += items_read * size;

    return items_read;
}

static size_t read_func_um3(void *ptr, size_t size, size_t nmemb, void * datasource)
{
    ogg_vorbis_streamfile * const ov_streamfile = datasource;
    size_t items_read;

    size_t bytes_read;
   
    bytes_read = read_streamfile(ptr, ov_streamfile->offset, size * nmemb,
            ov_streamfile->streamfile);

    items_read = bytes_read / size;

    /* first 0x800 bytes of um3 are xor'd with 0xff */
    if (ov_streamfile->offset < 0x800) {
        int num_crypt = 0x800-ov_streamfile->offset;
        int i;

        if (num_crypt > bytes_read) num_crypt=bytes_read;
        for (i=0;i<num_crypt;i++)
            ((uint8_t*)ptr)[i] ^= 0xff;
    }

    ov_streamfile->offset += items_read * size;

    return items_read;
}

static int seek_func(void *datasource, ogg_int64_t offset, int whence) {
    ogg_vorbis_streamfile * const ov_streamfile = datasource;
    ogg_int64_t base_offset;
    ogg_int64_t new_offset;

    switch (whence) {
        case SEEK_SET:
            base_offset = 0;
            break;
        case SEEK_CUR:
            base_offset = ov_streamfile->offset;
            break;
        case SEEK_END:
            base_offset = ov_streamfile->size;
            break;
        default:
            return -1;
            break;
    }

    new_offset = base_offset + offset;
    if (new_offset < 0 || new_offset > ov_streamfile->size) {
        return -1;
    } else {
        ov_streamfile->offset = new_offset;
        return 0;
    }
}

static long tell_func(void * datasource) {
    ogg_vorbis_streamfile * const ov_streamfile = datasource;
    return ov_streamfile->offset;
}

/* setting close_func in ov_callbacks to NULL doesn't seem to work */
static int close_func(void * datasource) {
    return 0;
}

/* Ogg Vorbis, by way of libvorbisfile */

VGMSTREAM * init_vgmstream_ogg_vorbis(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    ov_callbacks callbacks;
    OggVorbis_File temp_ovf;
    ogg_vorbis_streamfile temp_streamfile;

    ogg_vorbis_codec_data * data = NULL;
    OggVorbis_File *ovf;
    int inited_ovf = 0;
    vorbis_info *info;

    int loop_flag = 0;
    int32_t loop_start = 0;
    int loop_length_found = 0;
    int32_t loop_length = 0;
    int loop_end_found = 0;
    int32_t loop_end = 0;

    int um3_ogg = 0;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    
    /* It is only interesting to use oggs with vgmstream if they are looped.
       To prevent such files from being played by other plugins and such they
       may be renamed to .logg. This meta reader should still support .ogg,
       though. */
    if (strcasecmp("logg",filename_extension(filename)) &&
            strcasecmp("ogg",filename_extension(filename))
       ) {
        if(!strcasecmp("um3",filename_extension(filename))) {
            um3_ogg = 1;
        }
        else goto fail;
    }

    /* not all um3-ogg are crypted */
    if (um3_ogg && read_32bitBE(0x0,streamFile)==0x4f676753) um3_ogg = 0;

    if (um3_ogg)
        callbacks.read_func = read_func_um3;
    else
        callbacks.read_func = read_func;
    callbacks.seek_func = seek_func;
    callbacks.close_func = close_func;
    callbacks.tell_func = tell_func;

    temp_streamfile.streamfile = streamFile;
    temp_streamfile.offset = 0;
    temp_streamfile.size = get_streamfile_size(temp_streamfile.streamfile);

    /* can we open this as a proper ogg vorbis file? */
    memset(&temp_ovf, 0, sizeof(temp_ovf));
    if (ov_test_callbacks(&temp_streamfile, &temp_ovf, NULL,
            0, callbacks)) goto fail;
    /* we have to close this as it has the init_vgmstream meta-reading
       STREAMFILE */
    ov_clear(&temp_ovf);

    /* proceed to open a STREAMFILE just for this stream */
    data = calloc(1,sizeof(ogg_vorbis_codec_data));
    if (!data) goto fail;

    data->ov_streamfile.streamfile = streamFile->open(streamFile,filename,
            STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!data->ov_streamfile.streamfile) goto fail;
    data->ov_streamfile.offset = 0;
    data->ov_streamfile.size = get_streamfile_size(data->ov_streamfile.streamfile);

    /* open the ogg vorbis file for real */
    if (ov_open_callbacks(&data->ov_streamfile, &data->ogg_vorbis_file, NULL,
                0, callbacks)) goto fail;
    ovf = &data->ogg_vorbis_file;
    inited_ovf = 1;

    data->bitstream = DEFAULT_BITSTREAM;

    info = ov_info(ovf,DEFAULT_BITSTREAM);

    /* grab the comments */
    {
        int i;
        vorbis_comment *comment;

        comment = ov_comment(ovf,DEFAULT_BITSTREAM);

        /* search for a "loop_start" comment */
        for (i=0;i<comment->comments;i++) {
            if (strstr(comment->user_comments[i],"loop_start=")==
                    comment->user_comments[i] ||
                strstr(comment->user_comments[i],"LOOP_START=")==
                    comment->user_comments[i] ||
                strstr(comment->user_comments[i],"COMMENT=LOOPPOINT=")==
                    comment->user_comments[i] ||
                strstr(comment->user_comments[i],"LOOPSTART=")==
                    comment->user_comments[i] ||
                strstr(comment->user_comments[i],"um3.stream.looppoint.start=")==
                    comment->user_comments[i] ||
                strstr(comment->user_comments[i],"LoopStart=")==
                    comment->user_comments[i]
                    ) {
                loop_start=atol(strrchr(comment->user_comments[i],'=')+1);
                if (loop_start >= 0)
                    loop_flag=1;
            }
            else if (strstr(comment->user_comments[i],"LOOPLENGTH=")==
                    comment->user_comments[i]) {
                loop_length=atol(strrchr(comment->user_comments[i],'=')+1);
                loop_length_found=1;
            }
            else if (strstr(comment->user_comments[i],"title=-lps")==
                    comment->user_comments[i]) {
                loop_start=atol(comment->user_comments[i]+10);
                if (loop_start >= 0)
                    loop_flag=1;
            }
            else if (strstr(comment->user_comments[i],"album=-lpe")==
                    comment->user_comments[i]) {
                loop_end=atol(comment->user_comments[i]+10);
                loop_flag=1;
                loop_end_found=1;
            }
            else if (strstr(comment->user_comments[i],"LoopEnd=")==
                    comment->user_comments[i]) {
						if(loop_flag) {
							loop_length=atol(strrchr(comment->user_comments[i],'=')+1)-loop_start;
							loop_length_found=1;
						}
            }
            else if (strstr(comment->user_comments[i],"lp=")==
                    comment->user_comments[i]) {
                sscanf(strrchr(comment->user_comments[i],'=')+1,"%d,%d",
                        &loop_start,&loop_end);
                loop_flag=1;
                loop_end_found=1;
            }
        }
    }

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(info->channels,loop_flag);
    if (!vgmstream) goto fail;

    /* store our fun extra datas */
    vgmstream->codec_data = data;

    /* fill in the vital statistics */
    vgmstream->channels = info->channels;
    vgmstream->sample_rate = info->rate;

    /* let's play the whole file */
    vgmstream->num_samples = ov_pcm_total(ovf,-1);

    if (loop_flag) {
        vgmstream->loop_start_sample = loop_start;
        if (loop_length_found)
            vgmstream->loop_end_sample = loop_start+loop_length;
        else if (loop_end_found)
            vgmstream->loop_end_sample = loop_end;
        else
            vgmstream->loop_end_sample = vgmstream->num_samples;
        vgmstream->loop_flag = loop_flag;

        if (vgmstream->loop_end_sample > vgmstream->num_samples)
            vgmstream->loop_end_sample = vgmstream->num_samples;
    }
    vgmstream->coding_type = coding_ogg_vorbis;
    vgmstream->layout_type = layout_ogg_vorbis;
    if (um3_ogg)
        vgmstream->meta_type = meta_um3_ogg;
    else
        vgmstream->meta_type = meta_ogg_vorbis;

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (data) {
        if (inited_ovf)
            ov_clear(&data->ogg_vorbis_file);
        if (data->ov_streamfile.streamfile)
            close_streamfile(data->ov_streamfile.streamfile);
        free(data);
    }
    if (vgmstream) {
        vgmstream->codec_data = NULL;
        close_vgmstream(vgmstream);
    }
    return NULL;
}

#endif
