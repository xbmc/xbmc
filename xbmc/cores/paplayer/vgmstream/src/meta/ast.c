#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_ast(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    coding_t coding_type;

    int codec_number;
    int channel_count;
    int loop_flag;

    size_t max_block;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ast",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x5354524D || /* "STRM" */
            read_16bitBE(0xa,streamFile)!=0x10 ||
            /* check that file = header (0x40) + data */
            read_32bitBE(4,streamFile)+0x40!=get_streamfile_size(streamFile))
        goto fail;
    
    /* check for a first block */
    if (read_32bitBE(0x40,streamFile)!=0x424C434B)  /* "BLCK" */
        goto fail;

    /* check type details */
    codec_number = read_16bitBE(8,streamFile);
    loop_flag = read_16bitBE(0xe,streamFile);
    channel_count = read_16bitBE(0xc,streamFile);
    max_block = read_32bitBE(0x20,streamFile);

    switch (codec_number) {
        case 0:
            coding_type = coding_NGC_AFC;
            break;
        case 1:
            coding_type = coding_PCM16BE;
            break;
        default:
            goto fail;
    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(0x14,streamFile);
    vgmstream->sample_rate = read_32bitBE(0x10,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = read_32bitBE(0x18,streamFile);
    vgmstream->loop_end_sample = read_32bitBE(0x1c,streamFile);

    vgmstream->coding_type = coding_type;
    vgmstream->layout_type = layout_ast_blocked;
    vgmstream->meta_type = meta_AST;

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    (i==0?
                     max_block+0x20-4: /* first buffer a bit bigger to 
                                         read block header without
                                         inefficiency */
                     max_block
                    )
                    );

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

    /* start me up */
    ast_block_update(0x40,vgmstream);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
