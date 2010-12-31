#ifndef CDGDATA_H
#define CDGDATA_H

// CDG Command Code
static const unsigned int CDG_COMMAND = 0x09;

// CDG Instruction Codes
static const unsigned int CDG_INST_MEMORY_PRESET      = 1;
static const unsigned int CDG_INST_BORDER_PRESET      = 2;
static const unsigned int CDG_INST_TILE_BLOCK         = 6;
static const unsigned int CDG_INST_SCROLL_PRESET      = 20;
static const unsigned int CDG_INST_SCROLL_COPY        = 24;
static const unsigned int CDG_INST_DEF_TRANSP_COL     = 28;
static const unsigned int CDG_INST_LOAD_COL_TBL_0_7   = 30;
static const unsigned int CDG_INST_LOAD_COL_TBL_8_15  = 31;
static const unsigned int CDG_INST_TILE_BLOCK_XOR     = 38;

// Bitmask for all CDG fields
static const unsigned int CDG_MASK = 0x3F;

// This is the size of the display as defined by the CDG specification.
// The pixels in this region can be painted, and scrolling operations
// rotate through this number of pixels.
static const unsigned int CDG_FULL_WIDTH  = 300;
static const unsigned int CDG_FULL_HEIGHT = 216;
static const unsigned int CDG_BORDER_WIDTH = 6;
static const unsigned int CDG_BORDER_HEIGHT = 12;

typedef struct
{
  char command;
  char instruction;
  char parityQ[2];
  char data[16];
  char parityP[4];
} SubCode;

typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char repeat;    // Only lower 4 bits are used, mask with 0x0F
  char filler[14];
} CDG_MemPreset;

typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char filler[15];
} CDG_BorderPreset;

typedef struct
{
  char color0;    // Only lower 4 bits are used, mask with 0x0F
  char color1;    // Only lower 4 bits are used, mask with 0x0F
  char row;       // Only lower 5 bits are used, mask with 0x1F
  char column;    // Only lower 6 bits are used, mask with 0x3F
  char tilePixels[12]; // Only lower 6 bits of each byte are used
} CDG_Tile;

typedef struct
{
  char colorSpec[16]; // AND with 0x3F3F to clear P and Q channel
} CDG_LoadColorTable;

typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char hScroll;   // Only lower 6 bits are used, mask with 0x3F
  char vScroll;    // Only lower 6 bits are used, mask with 0x3F
}
CDG_Scroll;

#endif // CDG_H
