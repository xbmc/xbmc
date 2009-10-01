//	This decoder is based on in_mp4 from the faad project
//	(http://faac.sourceforge.net)

#define WIN32_LEAN_AND_MEAN
#ifndef _LINUX
#include <windows.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <faad.h>
#define USE_TAGGING
#include <mp4ff.h>


#include "AACCodec.h"
#include "utils.h"
#include "demux.h"
#include "stream.h"

static int res_table[] = {
    16,
    24,
    32,
    0,
    0,
    16
};

int m_resolution=0; //  16 Bits Per Sample

int host_bigendian = 0;

struct seek_list
{
    struct seek_list *next;
    __int64 offset;
};

extern struct alac_file *create_alac(int samplesize, int numchannels);
extern void alac_set_info(struct alac_file *alac, char *inputbuffer);
extern void decode_frame(struct alac_file *alac, unsigned char *inbuffer,void *outbuffer, int *outputsize);

typedef struct state
{
    /* general stuff */
    faacDecHandle hDecoder;
    int samplerate;
    unsigned char channels;
    int bitspersample;
    int totaltime;
    int bitrate;
    AAC_OBJECT_TYPE objecttype;
    int filetype; /* 0: MP4; 1: AAC */
    int last_frame;
    __int64 last_offset;

    /* io callback stuff */
    AACOpenCallback Open;
    AACCloseCallback Close;
    AACReadCallback Read;
    AACSeekCallback Seek;
    AACFileSizeCallback Filesize;
    void *userData;

    /* MP4 stuff */
    mp4ff_t* mp4file;
    int mp4track;
    long numSamples;
    long sampleId;
    mp4ff_callback_t mp4cb;

    /* AAC stuff */
    long m_aac_bytes_into_buffer;
    long m_aac_bytes_consumed;
    __int64 m_file_offset;
    unsigned char *m_aac_buffer;
    int m_at_eof;
    double cur_pos_sec;
    int m_header_type;
    struct seek_list *m_head;
    struct seek_list *m_tail;
    unsigned long m_length;

    /* for gapless decoding */
    unsigned int useAacLength;
    unsigned int framesize;
    unsigned int initial;
    unsigned long timescale;

    char* replaygain_track_gain;
    char* replaygain_album_gain;
    char* replaygain_track_peak;
    char* replaygain_album_peak;

    // alac support
    stream_t* stream;
    demux_res_t* demux_res;
    struct alac_file *alac;
    FILE* file;
    unsigned char* szSampleBuffer;
} state;

char last_error_message[1024];

/* Function definitions */
void *decode_aac_frame(state *st, faacDecFrameInfo *frameInfo);

void SetErrorMessage(char* message)
{
  strcpy(last_error_message, message);
}

static int get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
                           uint32_t *sample_duration,
                           uint32_t *sample_byte_size)
{
    unsigned int duration_index_accum = 0;
    unsigned int duration_cur_index = 0;

    if (samplenum >= demux_res->num_sample_byte_sizes)
    {
        fprintf(stderr, "sample %i does not exist\n", samplenum);
        return 0;
    }

    if (!demux_res->num_time_to_samples)
    {
        fprintf(stderr, "no time to samples\n");
        return 0;
    }
    while ((demux_res->time_to_sample[duration_cur_index].sample_count + duration_index_accum)
            <= samplenum)
    {
        duration_index_accum += demux_res->time_to_sample[duration_cur_index].sample_count;
        duration_cur_index++;
        if (duration_cur_index >= demux_res->num_time_to_samples)
        {
            fprintf(stderr, "sample %i does not have a duration\n", samplenum);
            return 0;
        }
    }

    *sample_duration = demux_res->time_to_sample[duration_cur_index].sample_duration;
    *sample_byte_size = demux_res->sample_byte_size[samplenum];

    return 1;
}

int fill_buffer(state *st)
{
  int bread;

  if (st->m_aac_bytes_consumed > 0)
  {
    if (st->m_aac_bytes_into_buffer)
    {
      memmove((void*)st->m_aac_buffer, (void*)(st->m_aac_buffer + st->m_aac_bytes_consumed),
        st->m_aac_bytes_into_buffer*sizeof(unsigned char));
    }

    if (!st->m_at_eof)
    {
      bread = st->Read(st->userData, (void*)(st->m_aac_buffer + st->m_aac_bytes_into_buffer),
        st->m_aac_bytes_consumed);

      if (bread != st->m_aac_bytes_consumed)
        st->m_at_eof = 1;

      st->m_aac_bytes_into_buffer += bread;
    }

    st->m_aac_bytes_consumed = 0;

    if (st->m_aac_bytes_into_buffer > 3)
    {
      if (memcmp(st->m_aac_buffer, "TAG", 3) == 0)
        st->m_aac_bytes_into_buffer = 0;
    }
    if (st->m_aac_bytes_into_buffer > 11)
    {
      if (memcmp(st->m_aac_buffer, "LYRICSBEGIN", 11) == 0)
        st->m_aac_bytes_into_buffer = 0;
    }
    if (st->m_aac_bytes_into_buffer > 8)
    {
      if (memcmp(st->m_aac_buffer, "APETAGEX", 8) == 0)
        st->m_aac_bytes_into_buffer = 0;
    }
  }

  return 1;
}

void advance_buffer(state *st, int bytes)
{
  st->m_file_offset += bytes;
  st->m_aac_bytes_consumed = bytes;
  st->m_aac_bytes_into_buffer -= bytes;
}

int adts_parse(state *st, __int64 *bitrate, double *length)
{
  static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
  int frames, frame_length;
  int t_framelength = 0;
  int samplerate;
  double frames_per_sec, bytes_per_frame;

  /* Read all frames to ensure correct time and bitrate */
  for (frames = 0; /* */; frames++)
  {
    fill_buffer(st);

    if (st->m_aac_bytes_into_buffer > 7)
    {
      /* check syncword */
      if (!((st->m_aac_buffer[0] == 0xFF)&&((st->m_aac_buffer[1] & 0xF6) == 0xF0)))
        break;

      st->m_tail->offset = st->m_file_offset;
      st->m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
      st->m_tail = st->m_tail->next;
      st->m_tail->next = NULL;

      if (frames == 0)
        samplerate = sample_rates[(st->m_aac_buffer[2]&0x3c)>>2];

      frame_length = ((((unsigned int)st->m_aac_buffer[3] & 0x3)) << 11)
        | (((unsigned int)st->m_aac_buffer[4]) << 3) | (st->m_aac_buffer[5] >> 5);

      t_framelength += frame_length;

      if (frame_length > st->m_aac_bytes_into_buffer)
        break;

      advance_buffer(st, frame_length);
    } 
    else 
    {
      break;
    }
  }

  frames_per_sec = (double)samplerate/1024.0;
  if (frames != 0)
    bytes_per_frame = (double)t_framelength/(double)(frames*1000);
  else
    bytes_per_frame = 0;
  *bitrate = (__int64)(8. * bytes_per_frame * frames_per_sec + 0.5);
  if (frames_per_sec != 0)
    *length = (double)frames/frames_per_sec;
  else
    *length = 1;

  return 1;
}

int skip_id3v2_tag(state *st)
{
  unsigned char buf[10];
  int bread, tagsize = 0;

  bread = st->Read(st->userData, buf, 10);
  if (bread != 10) return -1;

  if (!memcmp(buf, "ID3", 3))
  {
    /* high bit is not used */
    tagsize = (buf[6] << 21) | (buf[7] << 14) | (buf[8] << 7) | (buf[9] << 0);

    tagsize += 10;
    st->Seek(st->userData, tagsize);
  } 
  else 
  {
    st->Seek(st->userData, 0);
  }

  return tagsize;
}

// AAC order : C, L, R, L", R", LFE
// XBOX order : L, R, L", R", C, LFE
static void remap_channels(unsigned char *data, unsigned int samples, unsigned int bps)
{
  unsigned int i;

  switch (bps)
  {
  case 8:
    {
      unsigned char r1, r2, r3, r4, r5, r6;
      for (i = 0; i < samples; i += 6)
      {
        r1 = data[i];   // C
        r2 = data[i+1]; // L
        r3 = data[i+2]; // R
        r4 = data[i+3]; // L"
        r5 = data[i+4]; // R"
        r6 = data[i+5]; // LFE
        data[i] = r2;
        data[i+1] = r3;
        data[i+2] = r4;
        data[i+3] = r5;
        data[i+4] = r1;
        data[i+5] = r6;
      }
    }
    break;

  case 16:
  default:
    {
      unsigned short r1, r2, r3, r4, r5, r6;
      unsigned short *sample_buffer = (unsigned short *)data;
      for (i = 0; i < samples; i += 6)
      {
        r1 = sample_buffer[i];
        r2 = sample_buffer[i+1];
        r3 = sample_buffer[i+2];
        r4 = sample_buffer[i+3];
        r5 = sample_buffer[i+4];
        r6 = sample_buffer[i+5];
        sample_buffer[i] = r2;
        sample_buffer[i+1] = r3;
        sample_buffer[i+2] = r4;
        sample_buffer[i+3] = r5;
        sample_buffer[i+4] = r1;
        sample_buffer[i+5] = r6;
      }
    }
    break;

  case 24:
  case 32:
    {
      unsigned int r1, r2, r3, r4, r5, r6;
      unsigned int *sample_buffer = (unsigned int *)data;
      for (i = 0; i < samples; i += 6)
      {
        r1 = sample_buffer[i];
        r2 = sample_buffer[i+1];
        r3 = sample_buffer[i+2];
        r4 = sample_buffer[i+3];
        r5 = sample_buffer[i+4];
        r6 = sample_buffer[i+5];
        sample_buffer[i] = r2;
        sample_buffer[i+1] = r3;
        sample_buffer[i+2] = r4;
        sample_buffer[i+3] = r5;
        sample_buffer[i+4] = r1;
        sample_buffer[i+5] = r6;
      }
    }
    break;
  }
}

int ReadMP4AAC(AACHandle handle, BYTE* pBuffer, int iSize)
{
  int l=0;
  void *sample_buffer;
  unsigned char *buffer;
  int buffer_size;
  faacDecFrameInfo frameInfo;

  state* st=(state*) handle;

  if (st->last_frame)
  {
    return AAC_READ_EOF;
  }
  else 
  {
    int rc;

    /* for gapless decoding */
    char *buf;
    long dur;
    unsigned int sample_count;
    unsigned int delay = 0;

    /* get acces unit from MP4 file */
    buffer = NULL;
    buffer_size = 0;

    dur = mp4ff_get_sample_duration(st->mp4file, st->mp4track, st->sampleId);
    rc = mp4ff_read_sample(st->mp4file, st->mp4track, st->sampleId++, &buffer,  &buffer_size);
    if (st->sampleId-1 == 1) dur = 0;
    if (rc == 0 || buffer == NULL)
    {
      st->last_frame = 1;
      sample_buffer = NULL;
      frameInfo.samples = 0;
    } 
    else
    {
      sample_buffer = faacDecDecode(st->hDecoder, &frameInfo,
        buffer, buffer_size);
    }
    if (frameInfo.error > 0)
    {
      SetErrorMessage(faacDecGetErrorMessage(frameInfo.error));
      st->last_frame = 1;
    }
    if (st->sampleId >= st->numSamples)
      st->last_frame = 1;

    if (buffer) free(buffer);

    if (st->useAacLength || (st->timescale != st->samplerate)) 
    {
      sample_count = frameInfo.samples;
    }
    else 
    {
      sample_count = (unsigned int)(dur * frameInfo.channels);

      if (!st->useAacLength && !st->initial && (st->sampleId < st->numSamples/2) && (dur*frameInfo.channels != frameInfo.samples))
      {
        //fprintf(stderr, "MP4 seems to have incorrect frame duration, using values from AAC data.\n");
        st->useAacLength = 1;
        sample_count = frameInfo.samples;
      }
    }

    if (st->initial && (sample_count < st->framesize*st->channels) && (frameInfo.samples > sample_count))
    {
      delay = frameInfo.samples - sample_count;
    }

    if ((sample_count > 0))
    {
      buf = (char *)sample_buffer;
      st->initial = 0;

      switch (res_table[m_resolution])
      {
      case 8:
        buf += delay;
        break;
      case 16:
      default:
        buf += delay * 2;
        break;
      case 24:
      case 32:
        buf += delay * 4;
        break;
      case 64:
        buf += delay * 8;
      }

      if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
        remap_channels(buf, sample_count, res_table[m_resolution]);

      if (res_table[m_resolution] == 24)
      {
        /* convert libfaad output (3 bytes packed in 4) */
        char *temp_buffer = convert3in4to3in3(buf, sample_count);
        memcpy((void*)buf, (void*)temp_buffer, sample_count*3);
        free(temp_buffer);
      }

      l = sample_count * res_table[m_resolution] / 8;

      if (l>iSize)
      {
        char msg[128];
        sprintf(msg, "Sample Buffer to small (size %i, %i needed)", iSize, l);
        SetErrorMessage(msg);
        return AAC_READ_BUFFER_TO_SMALL;
      }

      memcpy((void*)pBuffer, (void*)buf, l);

      /* VBR bitrate display */
      //if (m_vbr_display)
      //{
      //  seq_frames++;
      //  seq_bytes += frameInfo.bytesconsumed;
      //  if (seq_frames == (int)(floor((float)frameInfo.samplerate/(float)(sample_count/frameInfo.channels) + 0.5)))
      //  {
      //    module.SetInfo((int)floor(((float)seq_bytes*8.)/1000. + 0.5),
      //      (int)floor(frameInfo.samplerate/1000. + 0.5),
      //      st->channels, 1);

      //    seq_frames = 0;
      //    seq_bytes = 0;
      //  }
      //}
    }
  }
  return l;
}

int GetMetaDataFreeFormString(state* st, const char* name, char** value)
{
  unsigned __int8 *pValue=NULL;
  int i = 0;

  do {
    char *pName=NULL;

    if (mp4ff_meta_get_by_index(st->mp4file, i, (char**)&pName, (char**)&pValue))
    {
      if (strcmp(name, pName)==0)
      {
        *value=(char*)strdup(pValue);
        free(pName);
        free(pValue);
        return 1;
      }
      free(pName);
      free(pValue);
    }

    i++;
  } while (pValue!=NULL);

  return 0;
}
void *decode_aac_frame(state *st, faacDecFrameInfo *frameInfo)
{
  void *sample_buffer = NULL;

  do
  {
    fill_buffer(st);

    if (st->m_aac_bytes_into_buffer != 0)
    {
      sample_buffer = faacDecDecode(st->hDecoder, frameInfo,
        st->m_aac_buffer, st->m_aac_bytes_into_buffer);

      if (st->m_header_type != 1)
      {
        if (st->last_offset < st->m_file_offset)
        {
          st->m_tail->offset = st->m_file_offset;
          st->m_tail->next = (struct seek_list*)malloc(sizeof(struct seek_list));
          st->m_tail = st->m_tail->next;
          st->m_tail->next = NULL;
          st->last_offset = st->m_file_offset;
        }
      }

      advance_buffer(st, frameInfo->bytesconsumed);
    }
    else
    {
      break;
    }

  } while (!frameInfo->samples && !frameInfo->error);

  return sample_buffer;
}

int aac_seek(state *st, double seconds)
{
  int i, frames;
  int bread;
  struct seek_list *target = st->m_head;

  if (1 /*can_seek*/ && ((st->m_header_type == 1) || (seconds < st->cur_pos_sec)))
  {
    frames = (int)(seconds*((double)st->samplerate/(double)st->framesize) + 0.5);

    for (i = 0; i < frames; i++)
    {
      if (target->next)
        target = target->next;
      else
        return 0;
    }
    if (target->offset == 0 && frames > 0)
      return 0;
    st->Seek(st->userData, (int)target->offset);
    st->m_file_offset = target->offset;

    bread = st->Read(st->userData, st->m_aac_buffer, 768*6);
    if (bread != 768*6)
      st->m_at_eof = 1;
    else
      st->m_at_eof = 0;
    st->m_aac_bytes_into_buffer = bread;
    st->m_aac_bytes_consumed = 0;
    st->m_file_offset += bread;

    faacDecPostSeekReset(st->hDecoder, -1);

    return 1;
  }
  else
  {
    if (seconds > st->cur_pos_sec)
    {
      faacDecFrameInfo frameInfo;

      frames = (int)((seconds - st->cur_pos_sec)*((double)st->samplerate/(double)st->framesize));

      if (frames > 0)
      {
        for (i = 0; i < frames; i++)
        {
          memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
          decode_aac_frame(st, &frameInfo);

          if (frameInfo.error || (st->m_aac_bytes_into_buffer == 0))
          {
            if (frameInfo.error)
            {
              if (faacDecGetErrorMessage(frameInfo.error) != NULL)
                SetErrorMessage(faacDecGetErrorMessage(frameInfo.error));
            }
            return 0;
          }
        }
      }

      faacDecPostSeekReset(st->hDecoder, -1);
    }
    return 1;
  }
}

int ReadRawAAC(AACHandle handle, BYTE* pBuffer, int iSize)
{
  state* st=(state*) handle;
  int l=0;

  faacDecFrameInfo frameInfo;
  void *sample_buffer;

  memset(&frameInfo, 0, sizeof(faacDecFrameInfo));

  sample_buffer = decode_aac_frame(st, &frameInfo);

  if (frameInfo.error || (st->m_aac_bytes_into_buffer == 0))
  {
    if (frameInfo.error)
    {
      if (faacDecGetErrorMessage(frameInfo.error) != NULL)
        SetErrorMessage(faacDecGetErrorMessage(frameInfo.error));

      return AAC_READ_ERROR;
    }

    return AAC_READ_EOF;
  }

  if ((frameInfo.samples > 0))
  {
    if (frameInfo.channels == 6 && frameInfo.num_lfe_channels)
      remap_channels(sample_buffer, frameInfo.samples, res_table[m_resolution]);

    if (res_table[m_resolution] == 24)
    {
      /* convert libfaad output (3 bytes packed in 4 bytes) */
      char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
      memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
      free(temp_buffer);
    }

    l = frameInfo.samples * res_table[m_resolution] / 8;

    if (l>iSize)
    {
      char msg[128];
      sprintf(msg, "Sample Buffer to small (size %i, %i needed)", iSize, l);
      SetErrorMessage(msg);
      return AAC_READ_BUFFER_TO_SMALL;
    }
    memcpy((void*)pBuffer, (void*)sample_buffer, l);

    /* VBR bitrate display */
    //if (m_vbr_display)
    //{
    //  seq_frames++;
    //  seq_bytes += frameInfo.bytesconsumed;
    //  if (seq_frames == (int)(floor((float)frameInfo.samplerate/(float)(frameInfo.samples/frameInfo.channels) + 0.5)))
    //  {
    //    module.SetInfo((int)floor(((float)seq_bytes*8.)/1000. + 0.5),
    //      (int)floor(frameInfo.samplerate/1000. + 0.5),
    //      st->channels, 1);

    //    seq_frames = 0;
    //    seq_bytes = 0;
    //  }
    //}
  }

  if (frameInfo.channels > 0 && st->samplerate > 0)
    st->cur_pos_sec += ((double)(frameInfo.samples/frameInfo.channels))/(double)st->samplerate;

  return l;
}

AACHandle AACAPI AACOpen(const char *fn, AACIOCallbacks callbacks)
{
  int avg_bitrate;
  unsigned char *buffer;
  int buffer_size;
  unsigned char header[8];
  faacDecConfigurationPtr config;
  state *st;
  unsigned int i;
  
  st = malloc(sizeof(struct state));

  if (!st)
    return AAC_INVALID_HANDLE;

  memset(st, 0, sizeof(state));

  st->Open=callbacks.Open;
  st->Close=callbacks.Close;
  st->Read=callbacks.Read;
  st->Seek=callbacks.Seek;
  st->userData=callbacks.userData;
  st->stream = NULL;
  st->demux_res = NULL;

  st->file = fopen(fn,"rb");
  if (st->file)
    st->stream = stream_create_file(st->file, 1); // we might have a alac file
  if (st->stream)
  {
    st->demux_res = (demux_res_t*)malloc(sizeof(demux_res_t));
	  memset( st->demux_res, 0, sizeof( demux_res_t ) );	// clear so all internal pointers are zero'd..
    if (qtmovie_read(st->stream, st->demux_res) && st->demux_res->format == 1634492771 && st->demux_res->sample_size == 16)
    {
      st->alac = create_alac(st->demux_res->sample_size, st->demux_res->num_channels);
      alac_set_info(st->alac, st->demux_res->codecdata);
      st->filetype = 0;
      st->bitspersample = st->demux_res->sample_size;
      st->bitrate = 128;
      st->samplerate = st->demux_res->sample_rate;
      st->channels = (unsigned char)st->demux_res->num_channels;
      st->totaltime = 0;
      st->m_aac_bytes_consumed = 1;
      st->m_aac_bytes_into_buffer = 1;
      st->m_file_offset = ftell(st->file);
      for (i= 0; i < st->demux_res->num_sample_byte_sizes; i++)
      {
        unsigned int thissample_duration;
        unsigned int thissample_bytesize;

        get_sample_info(st->demux_res, i, &thissample_duration,
          &thissample_bytesize);
        st->totaltime += thissample_duration*1000/(st->demux_res->sample_rate);
      }

      st->mp4track = 0;
      st->m_aac_buffer = (unsigned char*)malloc(1024*64);
      st->szSampleBuffer = (unsigned char*)malloc(1024*16);
      return st;
    }
    else
    {
      fclose(st->file);
      st->file = NULL;
      free(st->demux_res);
      st->demux_res = NULL;
      free(st->stream);
      st->stream = NULL;
    }
  }

  if (!st->Open(fn, "rb", st->userData))
  {
    SetErrorMessage("File open error.");
    free(st);
    return AAC_INVALID_HANDLE;
  }
  st->Read(st->userData, header, 8);
  st->Seek(st->userData, 0);
  st->filetype = 1;
  if (header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p')
      st->filetype = 0;

  if (st->filetype)
  {
    int tagsize = 0, tmp = 0, init;
    int bread = 0;
    double length = 0.;
    __int64 bitrate = 128;

    tagsize = skip_id3v2_tag(st);
    if (tagsize<0)
    {
      SetErrorMessage("Error reading id3 tag size.");
      free(st);
      return AAC_INVALID_HANDLE;
    }

    if (!(st->m_aac_buffer = (unsigned char*)malloc(768*6)))
    {
      SetErrorMessage("Memory allocation error.");
      free(st);
      return AAC_INVALID_HANDLE;
    }

    for (init=0; init<2; init++)
    {
      st->hDecoder = faacDecOpen();
      if (!st->hDecoder)
      {
        SetErrorMessage("Unable to open decoder library.");
        free(st);
        return AAC_INVALID_HANDLE;
      }

      config = faacDecGetCurrentConfiguration(st->hDecoder);
      config->outputFormat = m_resolution + 1;
      config->downMatrix = 0;
      faacDecSetConfiguration(st->hDecoder, config);

      memset(st->m_aac_buffer, 0, 768*6);
      bread = st->Read(st->userData, st->m_aac_buffer, 768*6);
      st->m_aac_bytes_into_buffer = bread;
      st->m_aac_bytes_consumed = 0;
      st->m_file_offset = 0;
      st->m_at_eof = (bread != 768*6) ? 1 : 0;

      if (init==0)
      {
        faacDecFrameInfo frameInfo;

        fill_buffer(st);

        if ((st->m_aac_bytes_consumed = faacDecInit(st->hDecoder,
          st->m_aac_buffer, st->m_aac_bytes_into_buffer,
          &st->samplerate, &st->channels)) < 0)
        {
          SetErrorMessage("Can't initialize decoder library.");
          free(st);
          return AAC_INVALID_HANDLE;
        }
        advance_buffer(st, st->m_aac_bytes_consumed);

        do {
          memset(&frameInfo, 0, sizeof(faacDecFrameInfo));
          fill_buffer(st);

          faacDecDecode(st->hDecoder, &frameInfo, st->m_aac_buffer, st->m_aac_bytes_into_buffer);
        } while (!frameInfo.samples && !frameInfo.error);

        if (frameInfo.error)
        {
          SetErrorMessage(faacDecGetErrorMessage(frameInfo.error));
          free(st);
          return AAC_INVALID_HANDLE;
        }

        st->channels = frameInfo.channels;
        st->samplerate = frameInfo.samplerate;
        st->framesize = (frameInfo.channels != 0) ? frameInfo.samples/frameInfo.channels : 0;
        st->objecttype = frameInfo.object_type;
        /*
        sbr = frameInfo.sbr;
        profile = frameInfo.object_type;
        header_type = frameInfo.header_type;
        */

        faacDecClose(st->hDecoder);
        st->Seek(st->userData, tagsize);
      }
    }

    st->m_head = (struct seek_list*)malloc(sizeof(struct seek_list));
    st->m_tail = st->m_head;
    st->m_tail->next = NULL;

    st->m_header_type = 0;
    if ((st->m_aac_buffer[0] == 0xFF) && ((st->m_aac_buffer[1] & 0xF6) == 0xF0))
    {
      if (1) //(can_seek)
      {
        adts_parse(st, &bitrate, &length);
        st->Seek(st->userData, tagsize);

        bread = st->Read(st->userData, st->m_aac_buffer, 768*6);
        if (bread != 768*6)
          st->m_at_eof = 1;
        else
          st->m_at_eof = 0;
        st->m_aac_bytes_into_buffer = bread;
        st->m_aac_bytes_consumed = 0;

        st->m_header_type = 1;
      }
    } 
    else if (memcmp(st->m_aac_buffer, "ADIF", 4) == 0) 
    {
      int skip_size = (st->m_aac_buffer[4] & 0x80) ? 9 : 0;
      bitrate = ((unsigned int)(st->m_aac_buffer[4 + skip_size] & 0x0F)<<19) |
        ((unsigned int)st->m_aac_buffer[5 + skip_size]<<11) |
        ((unsigned int)st->m_aac_buffer[6 + skip_size]<<3) |
        ((unsigned int)st->m_aac_buffer[7 + skip_size] & 0xE0);

      length = (double)st->Filesize(st->userData);
      if (length == -1)
      {
        length = 0;
      } 
      else 
      {
        length = ((double)length*8.)/((double)bitrate) + 0.5;
      }

      st->m_header_type = 2;
    } 
    else 
    {
      length = (double)st->Filesize(st->userData);
      length = ((double)length*8.)/((double)bitrate*1000.) + 0.5;
    }

    st->m_length = (int)(length*1000.);

    st->totaltime=st->m_length;

    fill_buffer(st);
    if ((st->m_aac_bytes_consumed = faacDecInit(st->hDecoder,
      st->m_aac_buffer, st->m_aac_bytes_into_buffer,
      &st->samplerate, &st->channels)) < 0)
    {
      SetErrorMessage("Can't initialize decoder library.");
      free(st);
      return AAC_INVALID_HANDLE;
    }
    advance_buffer(st, st->m_aac_bytes_consumed);

    if (st->m_header_type == 2)
      avg_bitrate = (int)bitrate;
    else
      avg_bitrate = (int)bitrate*1000;
  }
  else 
  {
    int64_t duration;
    double timescale_div;

    st->mp4cb.read=st->Read;
    st->mp4cb.write=NULL;
    st->mp4cb.seek=st->Seek;
    st->mp4cb.truncate=NULL;
    st->mp4cb.user_data=st->userData;
    st->mp4file = mp4ff_open_read(&st->mp4cb);
    if (!st->mp4file)
    {
      SetErrorMessage("Unable to open file.");
      faacDecClose(st->hDecoder);
      free(st);
      return AAC_INVALID_HANDLE;
    }

    if ((st->mp4track = GetAACTrack(st->mp4file)) < 0)
    {
      SetErrorMessage("Unsupported Audio track type.");
      mp4ff_close(st->mp4file);
      st->Close(st->userData);
      free(st);
      return AAC_INVALID_HANDLE;
    }

    st->hDecoder = faacDecOpen();
    if (!st->hDecoder)
    {
      SetErrorMessage("Unable to open decoder library.");
      free(st);
      return AAC_INVALID_HANDLE;
    }

    config = faacDecGetCurrentConfiguration(st->hDecoder);
    config->outputFormat = m_resolution + 1;
    config->downMatrix = 0;
    faacDecSetConfiguration(st->hDecoder, config);

    timescale_div = 1.0 / (double)mp4ff_time_scale(st->mp4file, st->mp4track);
    duration = mp4ff_get_track_duration_use_offsets(st->mp4file, st->mp4track);
    if (duration == -1)
    {
        st->totaltime = 0;
    } else {
        st->totaltime = (int)((double)duration * timescale_div * 1000.0);
    }

    buffer = NULL;
    buffer_size = 0;
    mp4ff_get_decoder_config(st->mp4file, st->mp4track,
        &buffer, &buffer_size);
    if (!buffer)
    {
      SetErrorMessage("mp4ff_get_decoder_config failed");
      faacDecClose(st->hDecoder);
      mp4ff_close(st->mp4file);
      st->Close(st->userData);
      free(st);
      return AAC_INVALID_HANDLE;
    }

    if(faacDecInit2(st->hDecoder, buffer, buffer_size,
      &st->samplerate, &st->channels) < 0)
    {
      SetErrorMessage("faacDecInit2 failed");
      /* If some error initializing occured, skip the file */
      faacDecClose(st->hDecoder);
      mp4ff_close(st->mp4file);
      st->Close(st->userData);
      if (buffer) free (buffer);
      free(st);
      return AAC_INVALID_HANDLE;
    }

    /* for gapless decoding */
    {
      mp4AudioSpecificConfig mp4ASC;

      st->timescale = mp4ff_time_scale(st->mp4file, st->mp4track);
      st->framesize = 1024;
      st->useAacLength = 0;

      if (buffer)
      {
        if (AudioSpecificConfig(buffer, buffer_size, &mp4ASC) >= 0)
        {
          if (mp4ASC.frameLengthFlag == 1) st->framesize = 960;
          if (mp4ASC.sbr_present_flag == 1) st->framesize *= 2;
        }
      }
    }

    free(buffer);

    avg_bitrate = mp4ff_get_avg_bitrate(st->mp4file, st->mp4track);

    st->numSamples = mp4ff_num_samples(st->mp4file, st->mp4track);
    st->sampleId = 1;

    {
      int rc;
      int32_t skip_samples = 0;
      faacDecFrameInfo frameInfo;
      void* sample_buffer;
      int buffer_size;

      rc = mp4ff_read_sample(st->mp4file, st->mp4track, st->sampleId, &buffer,  &buffer_size);

      if (rc == 0 || buffer == NULL)
      {
        SetErrorMessage("mp4ff_read_sample failed");
        /* If some error initializing occured, skip the file */
        faacDecClose(st->hDecoder);
        mp4ff_close(st->mp4file);
        st->Close(st->userData);
        if (buffer) free (buffer);
        free(st);
        return AAC_INVALID_HANDLE;
      } 
      sample_buffer = faacDecDecode(st->hDecoder, &frameInfo,
        buffer, buffer_size);

      if (frameInfo.error > 0)
      {
        SetErrorMessage(faacDecGetErrorMessage(frameInfo.error));
        /* If some error initializing occured, skip the file */
        faacDecClose(st->hDecoder);
        mp4ff_close(st->mp4file);
        st->Close(st->userData);
        if (buffer) free (buffer);
        free(st);
        return AAC_INVALID_HANDLE;
      }
      if (buffer) free(buffer);

      st->objecttype=frameInfo.object_type;
    }
  }

  if (st->channels == 0)
  {
    SetErrorMessage("Number of channels not supported for playback.");
    faacDecClose(st->hDecoder);
    if (st->filetype)
      st->Close(st->userData);
    else
    {
      mp4ff_close(st->mp4file);
      st->Close(st->userData);
    }
    free(st);
    return AAC_INVALID_HANDLE;
  }

  //if (m_downmix && (st->channels == 5 || st->channels == 6))
  //  st->channels = 2;

  st->bitspersample=res_table[m_resolution];

  st->bitrate = (int)floor(((float)avg_bitrate + 500.0)/1000.0 + 0.5)*1000;
  //sr = (int)floor((float)st->samplerate/1000.0 + 0.5);

  st->initial = 1;

  if (st->filetype==0)
  {
    //  Get replay gain for mp4
    GetMetaDataFreeFormString(st, "replaygain_track_gain", &st->replaygain_track_gain);
    GetMetaDataFreeFormString(st, "replaygain_track_peak", &st->replaygain_track_peak);
    GetMetaDataFreeFormString(st, "replaygain_album_gain", &st->replaygain_album_gain);
    GetMetaDataFreeFormString(st, "replaygain_album_peak", &st->replaygain_album_peak);
  }

  return st;
}

void AACAPI AACClose(AACHandle handle)
{
  state* st=(state*) handle;
  
  if (st->m_aac_buffer)
    free(st->m_aac_buffer);
  
  if (st->stream)
  {
    free(st->szSampleBuffer);
    free(st->demux_res->codecdata );
    free(st->demux_res->time_to_sample );
    free(st->demux_res->sample_byte_size );
    free(st->demux_res);
    fclose(st->file);
    free(st->stream);
  }
  else
  {
    struct seek_list *target = st->m_head;

    while (target)
    {
      struct seek_list *tmp = target;
      target = target->next;
      if (tmp) free(tmp);
    }
    faacDecClose(st->hDecoder);
    if (st->filetype)
      st->Close(st->userData);
    else
    {
      mp4ff_close(st->mp4file);
      st->Close(st->userData);
    }

    if (st->replaygain_track_gain)
      free(st->replaygain_track_gain);
    if (st->replaygain_album_gain)
      free(st->replaygain_album_gain);
    if (st->replaygain_track_peak)
      free(st->replaygain_track_peak);
    if (st->replaygain_album_peak)
      free(st->replaygain_album_peak);
  }
  free(st);
}

int AACAPI AACRead(AACHandle handle, BYTE* pBuffer, int iSize)
{
  state* st=(state*) handle;
  int iCopy;
  if (st->stream)
  {
    unsigned int sample_duration;
    unsigned int sample_byte_size;
    int iCurrSize = iSize;
    while (iCurrSize > 0)
    {
      if (st->m_aac_bytes_consumed == st->m_aac_bytes_into_buffer)
      {
        if (st->mp4track == st->demux_res->num_sample_byte_sizes+1)
        {
          iCurrSize = -1;
          break;
        }
        get_sample_info(st->demux_res,st->mp4track++,&sample_duration, &sample_byte_size);
        stream_read(st->stream, sample_byte_size,st->m_aac_buffer);
        decode_frame(st->alac, st->m_aac_buffer, st->szSampleBuffer,&st->m_aac_bytes_into_buffer);
        st->m_aac_bytes_consumed = 0;
      }
      if (iCurrSize > st->m_aac_bytes_into_buffer-st->m_aac_bytes_consumed)
        iCopy = st->m_aac_bytes_into_buffer-st->m_aac_bytes_consumed;
      else
        iCopy = iCurrSize;

      memcpy(pBuffer,st->szSampleBuffer+st->m_aac_bytes_consumed,iCopy);
      pBuffer += iCopy;
      iCurrSize -= iCopy;
      st->m_aac_bytes_consumed += iCopy;
    }
    if (iCurrSize == -1)
      return AAC_READ_EOF;

    return iSize-iCurrSize;
  }
  if (st->filetype)
  {
    return ReadRawAAC(handle, pBuffer, iSize);
  }
  else
  {
    return ReadMP4AAC(handle, pBuffer, iSize);
  }
}

int AACAPI AACSeek(AACHandle handle, int iTimeMs)
{
  state* st=(state*) handle;

  if (st->filetype)
  {
    return aac_seek(st, (double)iTimeMs/1000);
  }
  else
  {
    int64_t duration;
    int32_t skip_samples = 0;
    int64_t currpos=0;

    duration = (int64_t)(iTimeMs/1000.0 * st->samplerate + 0.5);
    if (st->stream)
    {
      unsigned int sample_duration=0;
      unsigned int sample_byte_size=0;
      int sec2byte = st->demux_res->sample_rate*st->demux_res->sample_size/8*st->demux_res->num_channels;
      fseek(st->file,st->m_file_offset,SEEK_SET);

      for( st->mp4track=0;currpos+1000*sample_duration/st->demux_res->sample_rate<iTimeMs&&st->mp4track<st->demux_res->num_sample_byte_sizes+1;++st->mp4track)
      {
        currpos+=sample_duration*1000/st->demux_res->sample_rate;
        fseek(st->file,sample_byte_size,SEEK_CUR);
        get_sample_info(st->demux_res,st->mp4track,&sample_duration, &sample_byte_size);
      }

      stream_read(st->stream, sample_byte_size,st->m_aac_buffer);
      decode_frame(st->alac, st->m_aac_buffer, st->szSampleBuffer,&st->m_aac_bytes_into_buffer);
      st->m_aac_bytes_consumed = (iTimeMs-currpos)/1000*st->demux_res->sample_rate*st->demux_res->sample_size/8*st->demux_res->num_channels;
      return 1;
    }
    else
    {
      st->sampleId = mp4ff_find_sample_use_offsets(st->mp4file,
        st->mp4track, duration, &skip_samples);
      if (st->sampleId<0)
      {
        st->sampleId=0;
        return 0;
      }
    }

    return 1;
  }
}

AACAPI const char* AACGetErrorMessage()
{
  return last_error_message;
}

int AACAPI AACGetInfo(AACHandle handle, AACInfo* info)
{
  state* st=(state*) handle;

  if (st->bitrate ==0 || st->bitspersample==0 || 
      st->channels==0 || st->samplerate==0 || 
      st->totaltime==0 )
    return 0;

  info->bitrate=st->bitrate;
  info->bitspersample=st->bitspersample;
  info->channels=st->channels;
  info->samplerate=st->samplerate;
  info->totaltime=st->totaltime;

  if (st->stream)
    info->objecttype = 24;
  else
    info->objecttype=st->objecttype;


  info->replaygain_track_gain=st->replaygain_track_gain;
  info->replaygain_album_gain=st->replaygain_album_gain;
  info->replaygain_track_peak=st->replaygain_track_peak;
  info->replaygain_album_peak=st->replaygain_album_peak;
  return 1;
}

