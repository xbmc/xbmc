#ifndef __MPEG_H__
#define __MPEG_H__

#include "srtypes.h"

typedef struct mp3_headerSt
{
  int lay;
  int version;
  int error_protection;
  int bitrate_index;
  int sampling_frequency;
  int padding;
  int extension;
  int mode;
  int mode_ext;
  int copyright;
  int original;
  int emphasis;
  int stereo;
} mp3_header_t;


extern error_code mpeg_find_first_header(const char* buffer, int size, int min_good_frames, int *frame_pos);
extern error_code mpeg_find_last_header(const char *buffer, int size, int min_good_frames, int *frame_pos);

#endif //__MPEG_H__

