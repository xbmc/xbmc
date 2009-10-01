#include "../vgmstream.h"
#include "meta.h"
#include "../util.h"
#include "../coding/acm_decoder.h"

/* InterPlay ACM */
/* The real work is done by libacm */
VGMSTREAM * init_vgmstream_acm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    ACMStream *acm_stream = NULL;
    mus_acm_codec_data *data;

    char filename[260];

    int loop_flag = 0;
	int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("acm",filename_extension(filename))) goto fail;

	/* check header */
    if (read_32bitBE(0x0,streamFile) != 0x97280301)
		goto fail;

    data = calloc(1,sizeof(mus_acm_codec_data));
    if (!data) goto fail;

    data->files = calloc(1,sizeof(ACMStream *));
    if (!data->files) {
        free(data); data = NULL;
        goto fail;
    }

    /* gonna do this a little backwards, open and parse the file
       before creating the vgmstream */

    if (acm_open_decoder(&acm_stream,streamFile,filename) != ACM_OK) {
        goto fail;
    }

    channel_count = acm_stream->info.channels;
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->channels = channel_count;
    vgmstream->sample_rate = acm_stream->info.rate;
    vgmstream->coding_type = coding_ACM;
    vgmstream->num_samples = acm_stream->total_values / acm_stream->info.channels;
    vgmstream->layout_type = layout_acm;
    vgmstream->meta_type = meta_ACM;

    data->file_count = 1;
    data->current_file = 0;
    data->files[0] = acm_stream;
    /*data->end_file = -1;*/

    vgmstream->codec_data = data;

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
