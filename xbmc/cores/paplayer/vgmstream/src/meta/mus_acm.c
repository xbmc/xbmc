#include "../vgmstream.h"
#include "meta.h"
#include "../util.h"
#include "../streamfile.h"
#include "../coding/acm_decoder.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

#define NAME_LENGTH 260

int exists(char *filename, STREAMFILE *streamfile) {
    STREAMFILE * temp = 
        streamfile->open(streamfile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!temp) return 0;

    close_streamfile(temp);
    return 1;
}

/* needs the name of a file in the directory to test, as all we can do reliably is attempt to open a file */
int find_directory_name(char *name_base, char *dir_name, int subdir_name_size, char *subdir_name, char *name, char *file_name, STREAMFILE *streamfile) {
    /* find directory name */
    {
        char temp_dir_name[NAME_LENGTH];

        subdir_name[0]='\0';
        concatn(subdir_name_size,subdir_name,name_base);

        if (strlen(subdir_name) >= subdir_name_size-2) goto fail;
        subdir_name[strlen(subdir_name)+1]='\0';
        subdir_name[strlen(subdir_name)]=DIRSEP;

        temp_dir_name[0]='\0';
        concatn(sizeof(temp_dir_name),temp_dir_name,dir_name);
        concatn(sizeof(temp_dir_name),temp_dir_name,subdir_name);
        concatn(sizeof(temp_dir_name),temp_dir_name,name_base);
        concatn(sizeof(temp_dir_name),temp_dir_name,name);
        concatn(sizeof(temp_dir_name),temp_dir_name,".ACM");

        if (!exists(temp_dir_name,streamfile)) {
            int i;
            /* try all lowercase */
            for (i=strlen(subdir_name)-1;i>=0;i--) {
                subdir_name[i]=tolower(subdir_name[i]);
            }
            temp_dir_name[0]='\0';
            concatn(sizeof(temp_dir_name),temp_dir_name,dir_name);
            concatn(sizeof(temp_dir_name),temp_dir_name,subdir_name);
            concatn(sizeof(temp_dir_name),temp_dir_name,name_base);
            concatn(sizeof(temp_dir_name),temp_dir_name,name);
            concatn(sizeof(temp_dir_name),temp_dir_name,".ACM");

            if (!exists(temp_dir_name,streamfile)) {
                /* try first uppercase */
                subdir_name[0]=toupper(subdir_name[0]);
                temp_dir_name[0]='\0';
                concatn(sizeof(temp_dir_name),temp_dir_name,dir_name);
                concatn(sizeof(temp_dir_name),temp_dir_name,subdir_name);
                concatn(sizeof(temp_dir_name),temp_dir_name,name_base);
                concatn(sizeof(temp_dir_name),temp_dir_name,name);
                concatn(sizeof(temp_dir_name),temp_dir_name,".ACM");
                if (!exists(temp_dir_name,streamfile)) {
                    /* try also 3rd uppercase */
                    subdir_name[2]=toupper(subdir_name[2]);
                    temp_dir_name[0]='\0';
                    concatn(sizeof(temp_dir_name),temp_dir_name,dir_name);
                    concatn(sizeof(temp_dir_name),temp_dir_name,subdir_name);
                    concatn(sizeof(temp_dir_name),temp_dir_name,name_base);
                    concatn(sizeof(temp_dir_name),temp_dir_name,name);
                    concatn(sizeof(temp_dir_name),temp_dir_name,".ACM");
                    
                    if (!exists(temp_dir_name,streamfile)) {
                        /* ah well, disaster has befallen your party */
                        goto fail;
                    }
                }
            }
        }
    }

    return 0;

fail:
    return 1;
}

/* MUS playlist for InterPlay ACM */
VGMSTREAM * init_vgmstream_mus_acm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    ACMStream *acm_stream = NULL;
    mus_acm_codec_data *data = NULL;

    char filename[NAME_LENGTH];
    char line_buffer[NAME_LENGTH];
    char * end_ptr;
    char name_base[NAME_LENGTH];
    char (*names)[NAME_LENGTH] = NULL;
    char dir_name[NAME_LENGTH];
    char subdir_name[NAME_LENGTH];

    int i;
    int loop_flag = 0;
	int channel_count;
    int file_count;
    size_t line_bytes;
    int whole_line_read = 0;
    off_t mus_offset = 0;

    int loop_end_index = -1;
    int loop_start_index = -1;
    int32_t loop_start_samples = -1;
    int32_t loop_end_samples = -1;

    int32_t total_samples = 0;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("mus",filename_extension(filename))) goto fail;

    /* read file name base */
    line_bytes = get_streamfile_dos_line(sizeof(line_buffer),line_buffer,
            mus_offset, streamFile, &whole_line_read);
    if (!whole_line_read) goto fail;
    mus_offset += line_bytes;
    memcpy(name_base,line_buffer,sizeof(name_base));

    /* uppercase name_base */
    {
        int i;
        for (i=0;name_base[i];i++) name_base[i]=toupper(name_base[i]);
    }

    /*printf("name base: %s\n",name_base);*/

    /* read track entry count */
    line_bytes = get_streamfile_dos_line(sizeof(line_buffer),line_buffer,
            mus_offset, streamFile, &whole_line_read);
    if (!whole_line_read) goto fail;
    if (line_buffer[0] == '\0') goto fail;
    mus_offset += line_bytes;
    file_count = strtol(line_buffer,&end_ptr,10);
    /* didn't parse whole line as an integer (optional opening whitespace) */
    if (*end_ptr != '\0') goto fail;

    /*printf("entries: %d\n",file_count);*/

    names = calloc(file_count,sizeof(names[0]));
    if (!names) goto fail;

    dir_name[0]='\0';
    concatn(sizeof(dir_name),dir_name,filename);

    {
        /* find directory name for the directory contianing the MUS */
        char * last_slash;
        last_slash = strrchr(dir_name,DIRSEP);
        if (last_slash != NULL) {
            /* trim off the file name */
            last_slash[1]='\0';
        } else {
            /* no dir name? annihilate! */
            dir_name[0] = '\0';
        }
    }

    /* can't do this until we have a file name */
    subdir_name[0]='\0';

    /* parse each entry */
    {
        char name[NAME_LENGTH];
        char loop_name_base_temp[NAME_LENGTH];
        char loop_name_temp[NAME_LENGTH];
        char loop_name_base[NAME_LENGTH];
        char loop_name[NAME_LENGTH];
        for (i=0;i<file_count;i++)
        {
            int fields_matched;
            line_bytes =
                get_streamfile_dos_line(sizeof(line_buffer),line_buffer,
                        mus_offset, streamFile, &whole_line_read);
            if (!whole_line_read) goto fail;
            mus_offset += line_bytes;

            fields_matched = sscanf(line_buffer,"%s %s %s",name,
                    loop_name_base_temp,loop_name_temp);


            if (fields_matched < 1) goto fail;
            if (fields_matched == 3 && loop_name_base_temp[0] != '@' && loop_name_temp[0] != '@')
            {
                int j;
                memcpy(loop_name,loop_name_temp,sizeof(loop_name));
                memcpy(loop_name_base,loop_name_base_temp,sizeof(loop_name_base));
                for (j=0;loop_name[j];j++) loop_name[j]=toupper(loop_name[j]);
                for (j=0;loop_name_base[j];j++) loop_name_base[j]=toupper(loop_name_base[j]);
                /* loop back entry */
                loop_end_index = i;
            }
            else if (fields_matched >= 2 && loop_name_base_temp[0] != '@')
            {
                int j;
                memcpy(loop_name,loop_name_base_temp,sizeof(loop_name));
                memcpy(loop_name_base,name_base,sizeof(loop_name_base));
                for (j=0;loop_name[j];j++) loop_name[j]=toupper(loop_name[j]);
                for (j=0;loop_name_base[j];j++) loop_name_base[j]=toupper(loop_name_base[j]);
                /* loop back entry */
                loop_end_index = i;
            }
            else
            {
                /* normal entry, ignoring the @TAG for now */
            }

            {
                /* uppercase */
                int j;
                for (j=0;j<strlen(name);j++) name[j]=toupper(name[j]);
            }

            /* try looking in the common directory */
            names[i][0] = '\0';
            concatn(sizeof(names[0]),names[i],dir_name);
            concatn(sizeof(names[0]),names[i],name);
            concatn(sizeof(names[0]),names[i],".ACM");

            if (!exists(names[i],streamFile)) {

                /* We can't test for the directory until we have a file name
                 * to look for, so we do it here with the first file that seems to
                 * be in a subdirectory */
                if (subdir_name[0]=='\0') {
                    if (find_directory_name(name_base, dir_name, sizeof(subdir_name), subdir_name, name, filename, streamFile))
                        goto fail;
                }

                names[i][0] = '\0';
                concatn(sizeof(names[0]),names[i],dir_name);
                concatn(sizeof(names[0]),names[i],subdir_name);
                concatn(sizeof(names[0]),names[i],name_base);
                concatn(sizeof(names[0]),names[i],name);
                concatn(sizeof(names[0]),names[i],".ACM");

                if (!exists(names[i],streamFile)) goto fail;
            }

            /*printf("%2d %s\n",i,names[i]);*/
        }

        if (loop_end_index != -1) {
            /* find the file to loop back to */
            char target_name[NAME_LENGTH];
            target_name[0]='\0';
            concatn(sizeof(target_name),target_name,dir_name);
            concatn(sizeof(target_name),target_name,subdir_name);
            concatn(sizeof(target_name),target_name,loop_name_base);
            concatn(sizeof(target_name),target_name,loop_name);
            concatn(sizeof(target_name),target_name,".ACM");
            /*printf("looking for loop %s\n",target_name);*/

            for (i=0;i<file_count;i++) {
                if (!strcmp(target_name,names[i]))
                {
                    loop_start_index = i;
                    break;
                }
            }

            if (loop_start_index != -1) {
                /*printf("loop from %d to %d\n",loop_end_index,loop_start_index);*/
                /*if (loop_start_index < file_count-1) loop_start_index++;*/
                loop_end_index++;
                loop_flag = 1;
            }

        }
    }

    /* set up the struct to track the files */
    data = calloc(1,sizeof(mus_acm_codec_data));
    if (!data) goto fail;

    data->files = calloc(file_count,sizeof(ACMStream *));
    if (!data->files) {
        free(data); data = NULL;
        goto fail;
    }

    /* open each file... */
    for (i=0;i<file_count;i++) {

        /* gonna do this a little backwards, open and parse the file
           before creating the vgmstream */

        if (acm_open_decoder(&acm_stream,streamFile,names[i]) != ACM_OK) {
            goto fail;
        }

        data->files[i]=acm_stream;

        if (i==loop_start_index) loop_start_samples = total_samples;
        if (i==loop_end_index)   loop_end_samples   = total_samples;

        total_samples += acm_stream->total_values / acm_stream->info.channels;

        if (i>0) {
            if (acm_stream->info.channels != data->files[0]->info.channels ||
                acm_stream->info.rate     != data->files[0]->info.rate) goto fail;
        }
    }

    if (i==loop_end_index)   loop_end_samples   = total_samples;

    channel_count = data->files[0]->info.channels;
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->channels = channel_count;
    vgmstream->sample_rate = data->files[0]->info.rate;
    vgmstream->coding_type = coding_ACM;
    vgmstream->num_samples = total_samples;
    vgmstream->loop_start_sample = loop_start_samples;
    vgmstream->loop_end_sample = loop_end_samples;
    vgmstream->layout_type = layout_mus_acm;
    vgmstream->meta_type = meta_MUS_ACM;

    data->file_count = file_count;
    data->current_file = 0;
    data->loop_start_file = loop_start_index;
    data->loop_end_file = loop_end_index;
    /*data->end_file = -1;*/

    vgmstream->codec_data = data;

    free(names);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (data) {
        int i;
        for (i=0;i<data->file_count;i++) {
            if (data->files[i]) {
                acm_close(data->files[i]);
                data->files[i] = NULL;
            }
        }
    }
    if (names) free(names);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
