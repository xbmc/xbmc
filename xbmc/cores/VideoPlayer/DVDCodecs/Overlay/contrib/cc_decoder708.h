#pragma once

#include <sys/stat.h>

extern "C"{
#include "cc_decoder.h"
}

#define CC708_MAX_PACKET_LENGTH 128 //According to EIA-708B, part 5
#define CC708_MAX_SERVICES 63
#define I708_MAX_ROWS 15
/*
 * This value should be 32, but there were 16-bit encoded samples (from Korea),
 * where RowCount calculated another way and equals 46 (23[8bit]*2)
 */
#define I708_MAX_COLUMNS (32 * 2)
#define I708_SCREENGRID_ROWS 75
#define I708_SCREENGRID_COLUMNS 210
#define I708_MAX_WINDOWS 8
#define CC708_NO_LAST_SEQUENCE -1

#define NTSC_CC_FIELD_1 0
#define NTSC_CC_FIELD_2 1
#define DTVCC_PACKET_DATA 2
#define DTVCC_PACKET_START 3

enum class CommandCodeC0
{
  NUL = 0x00,
  ETX = 0x03,
  BS = 0x08,
  FF = 0x0c,
  CR = 0x0d,
  HCR = 0x0e,
  EXT1 = 0x10,
  P16 = 0x18
};

enum class CommandCodeC1
{
  CW0 = 0x80,
  CW1 = 0x81,
  CW2 = 0x82,
  CW3 = 0x83,
  CW4 = 0x84,
  CW5 = 0x85,
  CW6 = 0x86,
  CW7 = 0x87,
  CLW = 0x88,
  DSW = 0x89,
  HDW = 0x8A,
  TGW = 0x8B,
  DLW = 0x8C,
  DLY = 0x8D,
  DLC = 0x8E,
  RST = 0x8F,
  SPA = 0x90,
  SPC = 0x91,
  SPL = 0x92,
  RSV93 = 0x93,
  RSV94 = 0x94,
  RSV95 = 0x95,
  RSV96 = 0x96,
  SWA = 0x97,
  DF0 = 0x98,
  DF1 = 0x99,
  DF2 = 0x9A,
  DF3 = 0x9B,
  DF4 = 0x9C,
  DF5 = 0x9D,
  DF6 = 0x9E,
  DF7 = 0x9F
};

struct commandC1
{
  CommandCodeC1 code;
  const char *name;
  const char *description;
  int length;
};


enum class WindowJustify
{
  LEFT = 0,
  RIGHT = 1,
  CENTER = 2,
  FULL = 3
};

enum class WindowPrintDirection
{
  LEFT_TO_RIGHT = 0,
  RIGHT_TO_LEFT = 1,
  TOP_TO_BOTTOM = 2,
  BOTTOM_TO_TOP = 3
};

enum class WindowScrollDirection
{
  LEFT_TO_RIGHT = 0,
  RIGHT_TO_LEFT = 1,
  TOP_TO_BOTTOM = 2,
  BOTTOM_TO_TOP = 3
};

enum class WindowScrollDisplayEffect
{
  SNAP = 0,
  FADE = 1,
  WIPE = 2
};

enum class WindowEffectDirection
{
  LEFT_TO_RIGHT = 0,
  RIGHT_TO_LEFT = 1,
  TOP_TO_BOTTOM = 2,
  BOTTOM_TO_TOP = 3
};

enum class WindowFillOpacity
{
  FO_SOLID = 0,
  FO_FLASH = 1,
  FO_TRANSLUCENT = 2,
  FO_TRANSPARENT = 3
};

enum class WindowBorderType
{
  NONE = 0,
  RAISED = 1,
  DEPRESSED = 2,
  UNIFORM = 3,
  SHADOW_LEFT = 4,
  SHADOW_RIGHT = 5
};

enum class PenSize
{
  SMALL = 0,
  STANDARD = 1,
  LARGE = 2
};

enum class PenFontStyle
{
  DEFAULT_OR_UNDEFINED = 0,
  MONOSPACED_WITH_SERIFS = 1,
  PROPORTIONALLY_SPACED_WITH_SERIFS = 2,
  MONOSPACED_WITHOUT_SERIFS = 3,
  PROPORTIONALLY_SPACED_WITHOUT_SERIFS = 4,
  CASUAL_FONT_TYPE = 5,
  CURSIVE_FONT_TYPE = 6,
  SMALL_CAPITALS = 7
};

enum class PenTextTag
{
  DIALOG = 0,
  SOURCE_OR_SPEAKER_ID = 1,
  ELECTRONIC_VOICE = 2,
  FOREIGN_LANGUAGE = 3,
  VOICEOVER = 4,
  AUDIBLE_TRANSLATION = 5,
  SUBTITLE_TRANSLATION = 6,
  VOICE_QUALITY_DESCRIPTION = 7,
  SONG_LYRICS = 8,
  SOUND_EFFECT_DESCRIPTION = 9,
  MUSICAL_SCORE_DESCRIPTION = 10,
  EXPLETIVE = 11,
  UNDEFINED_12 = 12,
  UNDEFINED_13 = 13,
  UNDEFINED_14 = 14,
  NOT_TO_BE_DISPLAYED = 15
};

enum class PenOffset
{
  SUBSCRIPT = 0,
  NORMAL = 1,
  SUPERSCRIPT = 2
};

enum class PenEdgeType
{
  NONE = 0,
  RAISED = 1,
  DEPRESSED = 2,
  UNIFORM = 3,
  SHADOW_LEFT = 4,
  SHADOW_RIGHT = 5
};

enum class PenAnchorPoint
{
  TOP_LEFT = 0,
  TOP_CENTER = 1,
  TOP_RIGHT = 2,
  MIDDLE_LEFT = 3,
  MIDDLE_CENTER = 4,
  MIDDLE_RIGHT = 5,
  BOTTOM_LEFT = 6,
  BOTTOM_CENTER = 7,
  BOTTOM_RIGHT = 8
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
  PenSize pen_size;
  int offset;
  PenTextTag text_tag;
  PenFontStyle font_tag;
  PenEdgeType edge_type;
  int underline;
  int italic;
} e708Pen_attribs;

typedef struct e708Window_attribs
{
  WindowJustify justify;
  WindowPrintDirection print_direction;
  WindowScrollDirection scroll_direction;
  int word_wrap;
  WindowScrollDisplayEffect display_effect;
  WindowEffectDirection effect_direction;
  int effect_speed;
  int fill_color;
  WindowFillOpacity fill_opacity;
  WindowBorderType border_type;
  int border_color;
} e708Window_attribs;

/**
 * Since 1-byte and 2-byte symbols could appear in captions and
 * since we have to keep symbols alignment and several windows could appear on a screen at one time,
 * we use special structure for holding symbols
 */
typedef struct e708_symbol
{
  unsigned short sym; //symbol itself, at least 16 bit
  unsigned char len; //length. could be 1 or 2
} e708_symbol;

#define CCX_DTVCC_SYM_SET(x, c) \
  { \
    x.len = 1; \
    x.sym = c; \
  }
#define CCX_DTVCC_SYM_SET_16(x, c1, c2) \
  { \
    x.len = 2; \
    x.sym = (c1 << 8) | c2; \
  }
#define CCX_DTVCC_SYM_IS_16(x) (x.len == 2)
#define CCX_DTVCC_SYM(x) ((unsigned char)(x.sym))
#define CCX_DTVCC_SYM_16_FIRST(x) ((unsigned char)(x.sym >> 8))
#define CCX_DTVCC_SYM_16_SECOND(x) ((unsigned char)(x.sym & 0xff))
#define CCX_DTVCC_SYM_IS_EMPTY(x) (x.len == 0)
#define CCX_DTVCC_SYM_IS_SET(x) (x.len > 0)

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
  PenAnchorPoint anchor_point;
  int col_count;
  int pen_style;
  int win_style;
  unsigned char commands[6]; // Commands used to create this window
  e708Window_attribs attribs;
  int pen_row;
  int pen_column;
  e708_symbol *rows[I708_MAX_ROWS];
  e708Pen_color pen_colors[I708_MAX_ROWS];
  e708Pen_attribs pen_attribs[I708_MAX_ROWS];
  int memory_reserved = 0;
  int is_empty;
} e708Window;

typedef struct tvscreen
{
  e708_symbol chars[I708_SCREENGRID_ROWS][I708_SCREENGRID_COLUMNS];
  e708Pen_color pen_colors[I708_SCREENGRID_ROWS];
  e708Pen_attribs pen_attribs[I708_SCREENGRID_ROWS];
  unsigned int cc_count;
  int service_number;
}
tvscreen;

class CDecoderCC708;
typedef struct cc708_service_decoder
{
  e708Window windows[I708_MAX_WINDOWS];
  int current_window;
  tvscreen tv;
  int cc_count;
  void *userdata;
  void (*callback)(int service, void *userdata);
  char text[I708_SCREENGRID_ROWS * I708_SCREENGRID_COLUMNS + 1];
  int textlen;
  CDecoderCC708 *parent;
}
cc708_service_decoder;

void process_character(cc708_service_decoder* decoder, e708_symbol symbol);

class CDecoderCC708
{
public:
  CDecoderCC708();
  virtual ~CDecoderCC708();
  void Init(void (*handler)(int service, void *userdata), void *userdata);
  void Decode(unsigned char *data, int datalength);
  bool m_servicesActive[CC708_MAX_SERVICES]{};
  cc708_service_decoder* m_cc708decoders;
  cc_decoder_t *m_cc608decoder;
  unsigned char m_current_packet[CC708_MAX_PACKET_LENGTH];
  int m_current_packet_length;
  bool m_is_current_packet_header_parsed;
  int m_last_sequence;
  bool m_seen708;
  bool m_seen608;
  bool m_no_rollup;
};
