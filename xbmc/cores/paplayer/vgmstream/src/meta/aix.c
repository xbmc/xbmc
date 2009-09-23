#include "../vgmstream.h"
#include "meta.h"
#include "../util.h"

typedef struct _AIXSTREAMFILE
{
  STREAMFILE sf;
  STREAMFILE *real_file;
  off_t start_physical_offset;
  off_t current_physical_offset;
  off_t current_logical_offset;
  off_t current_block_size;
  int stream_id;
} AIXSTREAMFILE;

static STREAMFILE *open_aix_with_STREAMFILE(STREAMFILE *file,off_t start_offset,int stream_id);

VGMSTREAM * init_vgmstream_aix(STREAMFILE *streamFile) {
    
	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileAIX = NULL;
    STREAMFILE * streamFileADX = NULL;
    char filename[260];
    off_t *segment_offset = NULL;
    int32_t *samples_in_segment = NULL;
    int32_t sample_count;

    int loop_flag = 0;
    int32_t loop_start_sample=0;
    int32_t loop_end_sample=0;

    aix_codec_data *data = NULL;

    off_t first_AIXP;
    off_t stream_list_offset;
    off_t stream_list_end;

    int stream_count,channel_count,segment_count;
    int sample_rate;

	int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("aix",filename_extension(filename))) goto fail;

    if (read_32bitBE(0x0,streamFile) != 0x41495846 ||   /* "AIXF" */
            read_32bitBE(0x08,streamFile) != 0x01000014 ||
            read_32bitBE(0x0c,streamFile) != 0x00000800)
        goto fail;

    first_AIXP = read_32bitBE(0x4,streamFile)+8;
    segment_count = (uint16_t)read_16bitBE(0x18,streamFile);
    stream_list_offset = 0x20+0x10*segment_count+0x10;

    if (stream_list_offset >= first_AIXP)
        goto fail;
    if (segment_count < 1)
        goto fail;

    sample_rate = read_32bitBE(stream_list_offset+8,streamFile);
    if (!check_sample_rate(sample_rate))
        goto fail;

    samples_in_segment = calloc(segment_count,sizeof(int32_t));
    if (!samples_in_segment)
        goto fail;
    segment_offset = calloc(segment_count,sizeof(off_t));
    if (!segment_offset)
        goto fail;

    for (i = 0; i < segment_count; i++)
    {
        segment_offset[i] = read_32bitBE(0x20+0x10*i+0,streamFile);
        samples_in_segment[i] = read_32bitBE(0x20+0x10*i+0x08,streamFile);
        /*printf("samples_in_segment[%d]=%d\n",i,samples_in_segment[i]);*/
        /* all segments must have equal samplerate */
        if (read_32bitBE(0x20+0x10*i+0x0c,streamFile) != sample_rate)
            goto fail;
    }

    if (segment_offset[0] != first_AIXP)
        goto fail;

    stream_count = (uint8_t)read_8bit(stream_list_offset,streamFile);
    if (stream_count < 1)
        goto fail;
    stream_list_end = stream_list_offset + 0x8 + stream_count * 8;

    if (stream_list_end >= first_AIXP)
        goto fail;

    channel_count = 0;
    for (i = 0; i < stream_count; i++)
    {
        /* all streams must have same samplerate as segments */
        if (read_32bitBE(stream_list_offset+8+i*8,streamFile)!=sample_rate)
            goto fail;
        channel_count += read_8bit(stream_list_offset+8+i*8+4,streamFile);
    }

    /* check for existence of segments */
    for (i = 0; i < segment_count; i++)
    {
        int j;
        off_t AIXP_offset = segment_offset[i];
        for (j = 0; j < stream_count; j++)
        {
            if (read_32bitBE(AIXP_offset,streamFile)!=0x41495850) /* "AIXP" */
                goto fail;
            if (read_8bit(AIXP_offset+8,streamFile)!=j)
                goto fail;
            AIXP_offset += read_32bitBE(AIXP_offset+4,streamFile)+8;
        }
    }

    /*streamFileAIX = streamFile->open(streamFile,filename,sample_rate*0.0375*2/32*18segment_count);*/
    streamFileAIX = streamFile->open(streamFile,filename,sample_rate*0.1*segment_count);
    if (!streamFileAIX) goto fail;

    data = malloc(sizeof(aix_codec_data));
    if (!data) goto fail;
    data->segment_count = segment_count;
    data->stream_count = stream_count;
    data->adxs = malloc(sizeof(STREAMFILE *)*segment_count*stream_count);
    if (!data->adxs) goto fail;
    for (i=0;i<segment_count*stream_count;i++) {
        data->adxs[i] = NULL;
    }
    data->sample_counts = calloc(segment_count,sizeof(int32_t));
    if (!data->sample_counts) goto fail;
    memcpy(data->sample_counts,samples_in_segment,segment_count*sizeof(int32_t));

    /* for each segment */
    for (i = 0; i < segment_count; i++)
    {
        int j;
        /* for each stream */
        for (j = 0; j < stream_count; j++)
        {
            VGMSTREAM *adx;
            /*printf("try opening segment %d/%d stream %d/%d %x\n",i,segment_count,j,stream_count,segment_offset[i]);*/
            streamFileADX = open_aix_with_STREAMFILE(streamFileAIX,segment_offset[i],j);
            if (!streamFileADX) goto fail;
            adx = data->adxs[i*stream_count+j] = init_vgmstream_adx(streamFileADX);
            if (!adx)
                goto fail;
            close_streamfile(streamFileADX); streamFileADX = NULL;

            if (adx->num_samples != data->sample_counts[i] ||
                    adx->loop_flag != 0)
                goto fail;

            /* save start things so we can restart for seeking/looping */
            /* copy the channels */
            memcpy(adx->start_ch,adx->ch,sizeof(VGMSTREAMCHANNEL)*adx->channels);
            /* copy the whole VGMSTREAM */
            memcpy(adx->start_vgmstream,adx,sizeof(VGMSTREAM));

        }
    }

    if (segment_count > 1)
    {
        loop_flag = 1;
    }

    sample_count = 0;
    for (i = 0; i < segment_count; i++)
    {
        sample_count += data->sample_counts[i];

        if (i == 0)
            loop_start_sample = sample_count;
        if (i == 1)
            loop_end_sample = sample_count;
    }

    vgmstream = allocate_vgmstream(channel_count,loop_flag);

    vgmstream->num_samples = sample_count;
    vgmstream->sample_rate = sample_rate;

    vgmstream->loop_start_sample = loop_start_sample;
    vgmstream->loop_end_sample = loop_end_sample;

    vgmstream->coding_type = data->adxs[0]->coding_type;
    vgmstream->layout_type = layout_aix;
    vgmstream->meta_type = meta_AIX;

    vgmstream->ch[0].streamfile = streamFileAIX;
    data->current_segment = 0;

    vgmstream->codec_data = data;
    free(segment_offset);
    free(samples_in_segment);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileAIX) close_streamfile(streamFileAIX);
    if (streamFileADX) close_streamfile(streamFileADX);
    if (vgmstream) close_vgmstream(vgmstream);
    if (samples_in_segment) free(samples_in_segment);
    if (segment_offset) free(segment_offset);
    if (data) {
        if (data->adxs)
        {
            int i;
            for (i=0;i<data->segment_count*data->stream_count;i++)
                if (data->adxs)
                    close_vgmstream(data->adxs[i]);
            free(data->adxs);
        }
        if (data->sample_counts)
        {
            free(data->sample_counts);
        }
        free(data);
    }
    return NULL;
}
static size_t read_aix(AIXSTREAMFILE *streamfile,uint8_t *dest,off_t offset,size_t length)
{
  size_t sz = 0;

  /*printf("trying to read %x bytes from %x (str%d)\n",length,offset,streamfile->stream_id);*/
  while (length > 0)
  {
      int read_something = 0;

      /* read the beginning of the requested block, if we can */
      if (offset >= streamfile->current_logical_offset)
      {
          off_t to_read;
          off_t length_available;

          length_available = 
              (streamfile->current_logical_offset+
               streamfile->current_block_size) - 
              offset;

          if (length < length_available)
          {
              to_read = length;
          }
          else
          {
              to_read = length_available;
          }

          if (to_read > 0)
          {
              size_t bytes_read;

              bytes_read = read_streamfile(dest,
                      streamfile->current_physical_offset+0x10+
                          (offset-streamfile->current_logical_offset),
                      to_read,streamfile->real_file);

              sz += bytes_read;
              if (bytes_read != to_read)
              {
                  /* an error which we will not attempt to handle here */
                  return sz;
              }

              read_something = 1;

              dest += bytes_read;
              offset += bytes_read;
              length -= bytes_read;
          }
      }

      if (!read_something)
      {
          /* couldn't read anything, must seek */
          int found_block = 0;

          /* as we have no memory we must start seeking from the beginning */
          if (offset < streamfile->current_logical_offset)
          {
              streamfile->current_logical_offset = 0;
              streamfile->current_block_size = 0;
              streamfile->current_physical_offset = 
                  streamfile->start_physical_offset;
          }

          /* seek ye forwards */
          while (!found_block) {
              /*printf("seek looks at %x\n",streamfile->current_physical_offset);*/
              switch (read_32bitBE(streamfile->current_physical_offset,
                          streamfile->real_file))
              {
                  case 0x41495850:  /* AIXP */
                      if (read_8bit(
                                  streamfile->current_physical_offset+8,
                                  streamfile->real_file) ==
                              streamfile->stream_id)
                      {
                          streamfile->current_block_size =
                              (uint16_t)read_16bitBE(
                                  streamfile->current_physical_offset+0x0a,
                                  streamfile->real_file);

                          if (offset >= streamfile->current_logical_offset+
                                  streamfile->current_block_size)
                          {
                              streamfile->current_logical_offset +=
                                  streamfile->current_block_size;
                          }
                          else
                          {
                              found_block = 1;
                          }
                      }

                      if (!found_block)
                      {
                          streamfile->current_physical_offset +=
                              read_32bitBE(
                                      streamfile->current_physical_offset+0x04,
                                      streamfile->real_file
                                      ) + 8;
                      }

                      break;
                  case 0x41495846:  /* AIXF */
                      /* shouldn't ever see this */
                  case 0x41495845:  /* AIXE */
                      /* shouldn't have reached the end o' the line... */
                  default:
                      return sz;
                      break;
              } /* end block/chunk type select */
          } /* end while !found_block */
      } /* end if !read_something */
  } /* end while length > 0 */
  
  return sz;
}

static void close_aix(AIXSTREAMFILE *streamfile)
{
    free(streamfile);
    return;
}

static size_t get_size_aix(AIXSTREAMFILE *streamfile)
{
  return 0;
}

static size_t get_offset_aix(AIXSTREAMFILE *streamfile)
{
  return streamfile->current_logical_offset;
}

static void get_name_aix(AIXSTREAMFILE *streamfile,char *buffer,size_t length)
{
  strncpy(buffer,"ARBITRARY.ADX",length);
  buffer[length-1]='\0';
}

static STREAMFILE *open_aix_impl(AIXSTREAMFILE *streamfile,const char * const filename,size_t buffersize) 
{
  AIXSTREAMFILE *newfile;
  if (strcmp(filename,"ARBITRARY.ADX"))
      return  NULL;

  newfile = malloc(sizeof(AIXSTREAMFILE));
  if (!newfile)
      return NULL;
  memcpy(newfile,streamfile,sizeof(AIXSTREAMFILE));
  return &newfile->sf;
}

static STREAMFILE *open_aix_with_STREAMFILE(STREAMFILE *file,off_t start_offset,int stream_id)
{
  AIXSTREAMFILE *streamfile = malloc(sizeof(AIXSTREAMFILE));

  if (!streamfile)
    return NULL;
  
  /* success, set our pointers */

  streamfile->sf.read = (void*)read_aix;
  streamfile->sf.get_size = (void*)get_size_aix;
  streamfile->sf.get_offset = (void*)get_offset_aix;
  streamfile->sf.get_name = (void*)get_name_aix;
  streamfile->sf.get_realname = (void*)get_name_aix;
  streamfile->sf.open = (void*)open_aix_impl;
  streamfile->sf.close = (void*)close_aix;
#ifdef PROFILE_STREAMFILE
  streamfile->sf.get_bytes_read = NULL;
  streamfile->sf.get_error_count = NULL;
#endif

  streamfile->real_file = file;
  streamfile->current_physical_offset = 
      streamfile->start_physical_offset = start_offset;
  streamfile->current_logical_offset = 0;
  streamfile->current_block_size = 0;
  streamfile->stream_id = stream_id;
  
  return &streamfile->sf;
}

