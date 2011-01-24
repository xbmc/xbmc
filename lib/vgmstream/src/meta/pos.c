#include <ctype.h>
#include "meta.h"
#include "../util.h"

#ifdef WIN32
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

/* .pos is a tiny file with loop points, and the same base name as a .wav */

VGMSTREAM * init_vgmstream_pos(STREAMFILE *streamFile) {
    
	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileWAV = NULL;
    char filename[260];
	char filenameWAV[260];

	int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("pos",filename_extension(filename))) goto fail;

	/* check for .WAV file */
	strcpy(filenameWAV,filename);
	strcpy(filenameWAV+strlen(filenameWAV)-3,"wav");

	streamFileWAV = streamFile->open(streamFile,filenameWAV,STREAMFILE_DEFAULT_BUFFER_SIZE);
	if (!streamFileWAV) {
        /* try again, ucase */
        for (i=strlen(filenameWAV);i>=0&&filenameWAV[i]!=DIRSEP;i--)
            filenameWAV[i]=toupper(filenameWAV[i]);

        streamFileWAV = streamFile->open(streamFile,filenameWAV,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!streamFileWAV) goto fail;
    }

    /* let the real initer do the parsing */
    vgmstream = init_vgmstream_riff(streamFileWAV);
    if (!vgmstream) goto fail;

    close_streamfile(streamFileWAV);
    streamFileWAV = NULL;

    /* install loops */
    if (!vgmstream->loop_flag) {
        vgmstream->loop_flag = 1;
        vgmstream->loop_ch = calloc(vgmstream->channels,
                sizeof(VGMSTREAMCHANNEL));
        if (!vgmstream->loop_ch) goto fail;
    }

    vgmstream->loop_start_sample = read_32bitLE(0,streamFile);
    vgmstream->loop_end_sample = read_32bitLE(4,streamFile);
    vgmstream->meta_type = meta_RIFF_WAVE_POS;

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileWAV) close_streamfile(streamFileWAV);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
