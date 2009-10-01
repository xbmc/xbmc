#include "meta.h"
#include "../util.h"
#include "../coding/nwa_decoder.h"
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

/* NWA - Visual Art's streams */

VGMSTREAM * init_vgmstream_nwa(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
	int i;
    int channel_count;
    int loop_flag = 0;
    int32_t loop_start_sample = 0;
    int32_t loop_end_sample = 0;
    int nwainfo_ini_found = 0;
    int gameexe_ini_found = 0;
    int just_pcm = 0;
    int comp_level = -2;
    nwa_codec_data *data = NULL;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("nwa",filename_extension(filename))) goto fail;

    channel_count = read_16bitLE(0x00,streamFile);
    if (channel_count != 1 && channel_count != 2) goto fail;

    /* check if we're using raw pcm */
    if (
            read_32bitLE(0x08,streamFile)==-1 || /* compression level */
            read_32bitLE(0x10,streamFile)==0  || /* block count */
            read_32bitLE(0x18,streamFile)==0  || /* compressed data size */
            read_32bitLE(0x20,streamFile)==0  || /* block size */
            read_32bitLE(0x24,streamFile)==0     /* restsize */
       )
    {
        just_pcm = 1;
    }
    else
    {
        comp_level = read_32bitLE(0x08,streamFile);

        data = malloc(sizeof(nwa_codec_data));
        if (!data) goto fail;

        data->nwa = open_nwa(streamFile,filename);
        if (!data->nwa) goto fail;
    }

    /* try to locate NWAINFO.INI in the same directory */
    {
        char ininame[260];
        char * ini_lastslash;
        char namebase_array[260];
        char *namebase;
        STREAMFILE *inistreamfile;

        /* here we assume that the "special encoding" does not affect
         * the directory separator */
        strncpy(ininame,filename,sizeof(ininame));
        ininame[sizeof(ininame)-1]='\0';    /* a pox on the stdlib! */

        streamFile->get_realname(streamFile,namebase_array,sizeof(namebase_array));

        ini_lastslash = strrchr(ininame,DIRSEP);
        if (!ini_lastslash) {
            strncpy(ininame,"NWAINFO.INI",sizeof(ininame));
            namebase = namebase_array;
        } else {
            strncpy(ini_lastslash+1,"NWAINFO.INI",
                    sizeof(ininame)-(ini_lastslash+1-ininame));
            namebase = strrchr(namebase_array,DIRSEP)+1;
        }
        ininame[sizeof(ininame)-1]='\0';    /* curse you, strncpy! */

        inistreamfile = streamFile->open(streamFile,ininame,4096);

        if (inistreamfile) {
            /* ini found, try to find our name */
            const char * ext;
            int length;
            int found;
            off_t offset;
            off_t file_size;
            off_t found_off = -1;

            nwainfo_ini_found = 1;

            ext = filename_extension(namebase);
            length = ext-1-namebase;
            file_size = get_streamfile_size(inistreamfile);

            for (found = 0, offset = 0; !found && offset<file_size; offset++) {
                off_t suboffset;
                /* Go for an n*m search 'cause it's easier than building an
                 * FSA for the search string. Just wanted to make the point that
                 * I'm not ignorant, just lazy. */
                for (suboffset = offset;
                        suboffset<file_size &&
                        suboffset-offset<length &&
                        read_8bit(suboffset,inistreamfile)==
                            namebase[suboffset-offset];
                        suboffset++) {}

                if (suboffset-offset==length &&
                        read_8bit(suboffset,inistreamfile)==0x09) { /* tab */
                    found=1;
                    found_off = suboffset+1;
                }
            }

            if (found) {
                char loopstring[9]={0};

                if (read_streamfile((uint8_t*)loopstring,found_off,8,
                            inistreamfile)==8)
                {
                    loop_start_sample = atol(loopstring);
                    if (loop_start_sample > 0) loop_flag = 1;
                }
            }   /* if found file name in INI */

            close_streamfile(inistreamfile);
        } /* if opened INI ok */
    } /* INI block */

    /* try to locate Gameexe.ini in the same directory */
    {
        char ininame[260];
        char * ini_lastslash;
        char namebase_array[260];
        char * namebase;
        STREAMFILE *inistreamfile;

        strncpy(ininame,filename,sizeof(ininame));
        ininame[sizeof(ininame)-1]='\0';    /* a pox on the stdlib! */

        streamFile->get_realname(streamFile,namebase_array,sizeof(namebase_array));

        ini_lastslash = strrchr(ininame,DIRSEP);
        if (!ini_lastslash) {
            strncpy(ininame,"Gameexe.ini",sizeof(ininame));
            namebase = namebase_array;
        } else {
            strncpy(ini_lastslash+1,"Gameexe.ini",
                    sizeof(ininame)-(ini_lastslash+1-ininame));
            namebase = strrchr(namebase_array,DIRSEP)+1;
        }
        ininame[sizeof(ininame)-1]='\0';    /* curse you, strncpy! */

        inistreamfile = streamFile->open(streamFile,ininame,4096);

        if (inistreamfile) {
            /* ini found, try to find our name */
            const char * ext;
            int length;
            int found;
            off_t offset;
            off_t file_size;
            off_t found_off = -1;

            gameexe_ini_found = 1;

            ext = filename_extension(namebase);
            length = ext-1-namebase;
            file_size = get_streamfile_size(inistreamfile);

            /* format of line is:
             * #DSTRACK = 00000000 - eeeeeeee - ssssssss = "name"    = "name2?"
             *                       ^22        ^33         ^45         ^57
             */

            for (found = 0, offset = 0; !found && offset<file_size; offset++) {
                off_t suboffset;
                uint8_t buf[10];

                if (read_8bit(offset,inistreamfile)!='#') continue;
                if (read_streamfile(buf,offset+1,10,inistreamfile)!=10) break;
                if (memcmp("DSTRACK = ",buf,10)) continue;
                if (read_8bit(offset+44,inistreamfile)!='\"') continue;

                for (suboffset = offset+45;
                        suboffset<file_size &&
                        suboffset-offset-45<length &&
                        tolower(read_8bit(suboffset,inistreamfile))==
                            tolower(namebase[suboffset-offset-45]);
                        suboffset++) {}

                if (suboffset-offset-45==length &&
                        read_8bit(suboffset,inistreamfile)=='\"') { /* tab */
                    found=1;
                    found_off = offset+22; /* loop end */
                }
            }

            if (found) {
                char loopstring[9]={0};
                int start_ok = 0, end_ok = 0;
                int32_t total_samples =
                    read_32bitLE(0x1c,streamFile)/channel_count;

                if (read_streamfile((uint8_t*)loopstring,found_off,8,
                            inistreamfile)==8)
                {
                    if (!memcmp("99999999",loopstring,8))
                    {
                        loop_end_sample = total_samples;
                    }
                    else
                    {
                        loop_end_sample = atol(loopstring);
                    }
                    end_ok = 1;
                }
                if (read_streamfile((uint8_t*)loopstring,found_off+11,8,
                            inistreamfile)==8)
                {
                    if (!memcmp("99999999",loopstring,8))
                    {
                        /* not ok to start at last sample,
                         * don't set start_ok flag */
                    }
                    else if (!memcmp("00000000",loopstring,8))
                    {
                        /* loops from the start aren't really loops */
                    }
                    else
                    {
                        loop_start_sample = atol(loopstring);
                        start_ok = 1;
                    }
                }

                if (start_ok && end_ok) loop_flag = 1;
            }   /* if found file name in INI */

            close_streamfile(inistreamfile);
        } /* if opened INI ok */
    } /* INI block */

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);

    vgmstream->num_samples = read_32bitLE(0x1c,streamFile)/channel_count;

    if (just_pcm) {
        switch (read_16bitLE(0x02,streamFile)) {
            case 8:
                vgmstream->coding_type = coding_PCM8;
                vgmstream->interleave_block_size = 1;
                break;
            case 16:
                vgmstream->coding_type = coding_PCM16LE;
                vgmstream->interleave_block_size = 2;
                break;
            default:
                goto fail;
        }
        if (channel_count > 1) {
            vgmstream->layout_type = layout_interleave;
        } else {
            vgmstream->layout_type = layout_none;
        }
    }
    else
    {
        switch (comp_level)
        {
            case 0:
                vgmstream->coding_type = coding_NWA0;
                break;
            case 1:
                vgmstream->coding_type = coding_NWA1;
                break;
            case 2:
                vgmstream->coding_type = coding_NWA2;
                break;
            case 3:
                vgmstream->coding_type = coding_NWA3;
                break;
            case 4:
                vgmstream->coding_type = coding_NWA4;
                break;
            case 5:
                vgmstream->coding_type = coding_NWA5;
                break;
            default:
                goto fail;
                break;
        }
        vgmstream->layout_type = layout_none;
    }

    if (nwainfo_ini_found) {
        vgmstream->meta_type = meta_NWA_NWAINFOINI;
        if (loop_flag) {
            vgmstream->loop_start_sample = loop_start_sample;
            vgmstream->loop_end_sample = vgmstream->num_samples;
        }
    } else if (gameexe_ini_found) {
        vgmstream->meta_type = meta_NWA_GAMEEXEINI;
        if (loop_flag) {
            vgmstream->loop_start_sample = loop_start_sample;
            vgmstream->loop_end_sample = loop_end_sample;
        }
    } else {
        vgmstream->meta_type = meta_NWA;
    }


    if (just_pcm) {
        /* open the file for reading by each channel */
        STREAMFILE *chstreamfile;

        /* have both channels use the same buffer, as interleave is so small */
        chstreamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

        if (!chstreamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = chstreamfile;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=0x2c+(off_t)(i*vgmstream->interleave_block_size);
        }
    }
    else
    {
        vgmstream->codec_data = data;
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    if (data) {
        if (data->nwa)
        {
            close_nwa(data->nwa);
        }
        free(data);
    }
    return NULL;
}
