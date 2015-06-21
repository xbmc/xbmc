#pragma once

#include <sys/stat.h>

extern "C"{
#include "cc_decoder.h"
}

#define MAX_708_PACKET_LENGTH   128
#define CCX_DECODERS_708_MAX_SERVICES 8
#define I708_MAX_ROWS 15
#define I708_MAX_COLUMNS 42
#define I708_SCREENGRID_ROWS 75
#define I708_SCREENGRID_COLUMNS 210
#define I708_MAX_WINDOWS 8

enum COMMANDS_C0_CODES
{
  NUL=0,
  ETX=3,
  BS=8,
  FF=0xC,
  CR=0xD,
  HCR=0xE,
  EXT1=0x10,
  P16=0x18
};

enum COMMANDS_C1_CODES
{
  CW0=0x80,
  CW1=0x81,
  CW2=0x82,
  CW3=0x83,
  CW4=0x84,
  CW5=0x85,
  CW6=0x86,
  CW7=0x87,
  CLW=0x88,
  DSW=0x89,
  HDW=0x8A,
  TGW=0x8B,
  DLW=0x8C,
  DLY=0x8D,
  DLC=0x8E,
  RST=0x8F,
  SPA=0x90,
  SPC=0x91,
  SPL=0x92,
  RSV93=0x93,
  RSV94=0x94,
  RSV95=0x95,
  RSV96=0x96,
  SWA=0x97,
  DF0=0x98,
  DF1=0x99,
  DF2=0x9A,
  DF3=0x9B,
  DF4=0x9C,
  DF5=0x9D,
  DF6=0x9E,
  DF7=0x9F
};

struct S_COMMANDS_C1
{
  int code;
  const char *name;
  const char *description;
  int length;
};


enum eWindowsAttribJustify
{
  left=0,
  right=1,
  center=2,
  full=3
};

enum eWindowsAttribPrintDirection
{
  pd_left_to_right=0,
  pd_right_to_left=1,
  pd_top_to_bottom=2,
  pd_bottom_to_top=3
};

enum eWindowsAttribScrollDirection
{
  sd_left_to_right=0,
  sd_right_to_left=1,
  sd_top_to_bottom=2,
  sd_bottom_to_top=3
};

enum eWindowsAttribScrollDisplayEffect
{
  snap=0,
  fade=1,
  wipe=2
};

enum eWindowsAttribEffectDirection
{
  left_to_right=0,
  right_to_left=1,
  top_to_bottom=2,
  bottom_to_top=3
};

enum eWindowsAttribFillOpacity
{
  solid=0,
  flash=1,
  traslucent=2,
  transparent=3
};

enum eWindowsAttribBorderType
{
  none=0,
  raised=1,
  depressed=2,
  uniform=3,
  shadow_left=4,
  shadow_right=5
};

enum ePenAttribSize
{
  pensize_small=0,
  pensize_standard=1,
  pensize_large=2
};

enum ePenAttribFontStyle
{
  fontstyle_default_or_undefined=0,
  monospaced_with_serifs=1,
  proportionally_spaced_with_serifs=2,
  monospaced_without_serifs=3,
  proportionally_spaced_without_serifs=4,
  casual_font_type=5,
  cursive_font_type=6,
  small_capitals=7
};

enum ePanAttribTextTag
{
  texttag_dialog=0,
  texttag_source_or_speaker_id=1,
  texttag_electronic_voice=2,
  texttag_foreign_language=3,
  texttag_voiceover=4,
  texttag_audible_translation=5,
  texttag_subtitle_translation=6,
  texttag_voice_quality_description=7,
  texttag_song_lyrics=8,
  texttag_sound_effect_description=9,
  texttag_musical_score_description=10,
  texttag_expletitive=11,
  texttag_undefined_12=12,
  texttag_undefined_13=13,
  texttag_undefined_14=14,
  texttag_not_to_be_displayed=15
};

enum ePanAttribOffset
{
  offset_subscript=0,
  offset_normal=1,
  offset_superscript=2
};

enum ePanAttribEdgeType
{
  edgetype_none=0,
  edgetype_raised=1,
  edgetype_depressed=2,
  edgetype_uniform=3,
  edgetype_left_drop_shadow=4,
  edgetype_right_drop_shadow=5
};

enum eAnchorPoints
{
  anchorpoint_top_left = 0,
  anchorpoint_top_center = 1,
  anchorpoint_top_right =2,
  anchorpoint_middle_left = 3,
  anchorpoint_middle_center = 4,
  anchorpoint_middle_right = 5,
  anchorpoint_bottom_left = 6,
  anchorpoint_bottom_center = 7,
  anchorpoint_bottom_right = 8
};

typedef struct e708Pen_color
{
  int fg_color;
  int fg_opacity;
  int bg_color;
  int bg_opacity;
  int edge_color;
} e708Pen_color;

typedef struct e708Pen_attribs
{
  int pen_size;
  int offset;
  int text_tag;
  int font_tag;
  int edge_type;
  int underline;
  int italic;
} e708Pen_attribs;

typedef struct e708Window_attribs
{
  int fill_color;
  int fill_opacity;
  int border_color;
  int border_type01;
  int justify;
  int scroll_dir;
  int print_dir;
  int word_wrap;
  int border_type;
  int display_eff;
  int effect_dir;
  int effect_speed;
} e708Window_attribs;

typedef struct e708Window
{
  int is_defined;
  int number; // Handy, in case we only have a pointer to the window
  int priority;
  int col_lock;
  int row_lock;
  int visible;
  int anchor_vertical;
  int relative_pos;
  int anchor_horizontal;
  int row_count;
  int anchor_point;
  int col_count;
  int pen_style;
  int win_style;
  unsigned char commands[6]; // Commands used to create this window
  e708Window_attribs attribs;
  e708Pen_attribs pen;
  e708Pen_color pen_color;
  int pen_row;
  int pen_column;
  unsigned char *rows[I708_MAX_ROWS+1]; // Max is 15, but we define an extra one for convenience
  int memory_reserved;
  int is_empty;
} e708Window;

typedef struct tvscreen
{
  unsigned char chars[I708_SCREENGRID_ROWS][I708_SCREENGRID_COLUMNS+1];
}
tvscreen;

class CDecoderCC708;
typedef struct cc708_service_decoder
{
  e708Window windows[I708_MAX_WINDOWS];
  int current_window;
  int inited;
  int service;
  tvscreen tv;
  int is_empty_tv;
  int srt_counter;
  void *userdata;
  void (*callback)(int service, void *userdata);
  char text[I708_SCREENGRID_ROWS*I708_SCREENGRID_COLUMNS+1];
  int textlen;
  CDecoderCC708 *parent;
}
cc708_service_decoder;

void process_character (cc708_service_decoder *decoder, unsigned char internal_char);

class CDecoderCC708
{
public:
  CDecoderCC708();
  virtual ~CDecoderCC708();
  void Init(void (*handler)(int service, void *userdata), void *userdata);
  void Decode(const unsigned char *data, int datalength);
  bool m_inited;
  cc708_service_decoder* m_cc708decoders;
  cc_decoder_t *m_cc608decoder;
  unsigned char m_current_packet[MAX_708_PACKET_LENGTH]; // Length according to EIA-708B, part 5
  int m_current_packet_length;
  int m_last_seq;
  bool m_seen708;
  bool m_seen608;
};
