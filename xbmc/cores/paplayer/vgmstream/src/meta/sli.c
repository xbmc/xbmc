#include "../vgmstream.h"

#ifdef VGM_USE_VORBIS

#include <ctype.h>
#include "meta.h"
#include "../util.h"

#ifdef WIN32
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

/* .sli is a file with loop points, associated with a similarly named .ogg */

VGMSTREAM * init_vgmstream_sli_ogg(STREAMFILE *streamFile) {
    
	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileOGG = NULL;
    char filename[260];
	char filenameOGG[260];
    char linebuffer[260];
    off_t bytes_read;
    off_t sli_offset;
    int done;
    int32_t loop_start = -1;
    int32_t loop_length = -1;
    int32_t loop_from = -1;
    int32_t loop_to = -1;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sli",filename_extension(filename))) goto fail;

	/* check for .OGG file */
	strcpy(filenameOGG,filename);
    /* strip off .sli */
    filenameOGG[strlen(filenameOGG)-4]='\0';

	streamFileOGG = streamFile->open(streamFile,filenameOGG,STREAMFILE_DEFAULT_BUFFER_SIZE);
	if (!streamFileOGG) {
        goto fail;
    }

    /* let the real initer do the parsing */
    vgmstream = init_vgmstream_ogg_vorbis(streamFileOGG);
    if (!vgmstream) goto fail;

    close_streamfile(streamFileOGG);
    streamFileOGG = NULL;

    sli_offset = 0;
    while ((loop_start == -1 || loop_length == -1) && sli_offset < get_streamfile_size(streamFile)) {
        char *endptr;
        char *foundptr;
        bytes_read=get_streamfile_dos_line(sizeof(linebuffer),linebuffer,sli_offset,streamFile,&done);
        if (!done) goto fail;

        if (!memcmp("LoopStart=",linebuffer,10) && linebuffer[10]!='\0') {
            loop_start = strtol(linebuffer+10,&endptr,10);
            if (*endptr != '\0') {
                /* if it didn't parse cleanly */
                loop_start = -1;
            }
        }
        else if (!memcmp("LoopLength=",linebuffer,11) && linebuffer[11]!='\0') {
            loop_length = strtol(linebuffer+11,&endptr,10);
            if (*endptr != '\0') {
                /* if it didn't parse cleanly */
                loop_length = -1;
            }
        }

        /* a completely different format, also with .sli extension and can be handled similarly */
        if ((foundptr=strstr(linebuffer,"To="))!=NULL && isdigit(foundptr[3])) {
            loop_to = strtol(foundptr+3,&endptr,10);
            if (*endptr != ';') {
                loop_to = -1;
            }
        }
        if ((foundptr=strstr(linebuffer,"From="))!=NULL && isdigit(foundptr[5])) {
            loop_from = strtol(foundptr+5,&endptr,10);
            if (*endptr != ';') {
                loop_from = -1;
            }
        }

        sli_offset += bytes_read;
    }

    if ((loop_start != -1 && loop_length != -1) ||
        (loop_to != -1 && loop_from != -1)) {
        /* install loops */
        if (!vgmstream->loop_flag) {
            vgmstream->loop_flag = 1;
            vgmstream->loop_ch = calloc(vgmstream->channels,
                    sizeof(VGMSTREAMCHANNEL));
            if (!vgmstream->loop_ch) goto fail;
        }

        if (loop_to != -1 && loop_from != -1) {
            vgmstream->loop_start_sample = loop_to;
            vgmstream->loop_end_sample = loop_from;
            vgmstream->meta_type = meta_OGG_SLI2;
        } else {
            vgmstream->loop_start_sample = loop_start;
            vgmstream->loop_end_sample = loop_start+loop_length;
            vgmstream->meta_type = meta_OGG_SLI;
        }
    } else goto fail; /* if there's no loop points the .sli wasn't valid */

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileOGG) close_streamfile(streamFileOGG);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
#endif
