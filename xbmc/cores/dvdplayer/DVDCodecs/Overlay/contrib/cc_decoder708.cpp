/*
 * this is mostly borrowed from ccextractor http://ccextractor.sourceforge.net/
 */

#include "cc_decoder708.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/********************************************************
256 BYTES IS ENOUGH FOR ALL THE SUPPORTED CHARACTERS IN
EIA-708, SO INTERNALLY WE USE THIS TABLE (FOR CONVENIENCE)

00-1F -> Characters that are in the G2 group in 20-3F,
         except for 06, which is used for the closed captions
         sign "CC" which is defined in group G3 as 00. (this
         is by the article 33).
20-7F -> Group G0 as is - corresponds to the ASCII code
80-9F -> Characters that are in the G2 group in 60-7F
         (there are several blank characters here, that's OK)
A0-FF -> Group G1 as is - non-English characters and symbols
*/

unsigned char get_internal_from_G0 (unsigned char g0_char)
{
    return g0_char;
}

unsigned char get_internal_from_G1 (unsigned char g1_char)
{
    return g1_char;
}

// TODO: Probably not right
// G2: Extended Control Code Set 1
unsigned char get_internal_from_G2 (unsigned char g2_char)
{
  // according to the comment a few lines above those lines are indeed wrong
  /*
    if (g2_char>=0x20 && g2_char<=0x3F)
        return g2_char-0x20;
    if (g2_char>=0x60 && g2_char<=0x7F)
        return g2_char+0x20;
  */
    // Rest unmapped, so we return a blank space
    return 0x20;
}

// TODO: Probably not right
// G3: Future Characters and Icon Expansion
unsigned char get_internal_from_G3 (unsigned char g3_char)
{
    if (g3_char==0xa0) // The "CC" (closed captions) sign
        return 0x06;
    // Rest unmapped, so we return a blank space
    return 0x20;
}

void clearTV (cc708_service_decoder *decoder);

const char *COMMANDS_C0[32]=
{
    "NUL", // 0 = NUL
    NULL,  // 1 = Reserved
    NULL,  // 2 = Reserved
    "ETX", // 3 = ETX
    NULL,  // 4 = Reserved
    NULL,  // 5 = Reserved
    NULL,  // 6 = Reserved
    NULL,  // 7 = Reserved
    "BS",  // 8 = Backspace
    NULL,  // 9 = Reserved
    NULL,  // A = Reserved
    NULL,  // B = Reserved
    "FF",  // C = FF
    "CR",  // D = CR
    "HCR", // E = HCR
    NULL,  // F = Reserved
    "EXT1",// 0x10 = EXT1,
    NULL,  // 0x11 = Reserved
    NULL,  // 0x12 = Reserved
    NULL,  // 0x13 = Reserved
    NULL,  // 0x14 = Reserved
    NULL,  // 0x15 = Reserved
    NULL,  // 0x16 = Reserved
    NULL,  // 0x17 = Reserved
    "P16", // 0x18 = P16
    NULL,  // 0x19 = Reserved
    NULL,  // 0x1A = Reserved
    NULL,  // 0x1B = Reserved
    NULL,  // 0x1C = Reserved
    NULL,  // 0x1D = Reserved
    NULL,  // 0x1E = Reserved
    NULL,  // 0x1F = Reserved
};

struct S_COMMANDS_C1 COMMANDS_C1[32]=
{
    {CW0,"CW0","SetCurrentWindow0",     1},
    {CW1,"CW1","SetCurrentWindow1",     1},
    {CW2,"CW2","SetCurrentWindow2",     1},
    {CW3,"CW3","SetCurrentWindow3",     1},
    {CW4,"CW4","SetCurrentWindow4",     1},
    {CW5,"CW5","SetCurrentWindow5",     1},
    {CW6,"CW6","SetCurrentWindow6",     1},
    {CW7,"CW7","SetCurrentWindow7",     1},
    {CLW,"CLW","ClearWindows",          2},
    {DSW,"DSW","DisplayWindows",        2},
    {HDW,"HDW","HideWindows",           2},
    {TGW,"TGW","ToggleWindows",         2},
    {DLW,"DLW","DeleteWindows",         2},
    {DLY,"DLY","Delay",                 2},
    {DLC,"DLC","DelayCancel",           1},
    {RST,"RST","Reset",                 1},
    {SPA,"SPA","SetPenAttributes",      3},
    {SPC,"SPC","SetPenColor",           4},
    {SPL,"SPL","SetPenLocation",        3},
    {RSV93,"RSV93","Reserved",          1},
    {RSV94,"RSV94","Reserved",          1},
    {RSV95,"RSV95","Reserved",          1},
    {RSV96,"RSV96","Reserved",          1},
    {SWA,"SWA","SetWindowAttributes",   5},
    {DF0,"DF0","DefineWindow0",         7},
    {DF1,"DF0","DefineWindow1",         7},
    {DF2,"DF0","DefineWindow2",         7},
    {DF3,"DF0","DefineWindow3",         7},
    {DF4,"DF0","DefineWindow4",         7},
    {DF5,"DF0","DefineWindow5",         7},
    {DF6,"DF0","DefineWindow6",         7},
    {DF7,"DF0","DefineWindow7",         7}
};

void clear_packet(cc708_service_decoder *decoder)
{
  decoder->parent->m_current_packet_length = 0;
}

void cc708_service_reset(cc708_service_decoder *decoder)
{
  // There's lots of other stuff that we need to do, such as canceling delays
  for (int j=0;j<8;j++)
  {
    decoder->windows[j].is_defined=0;
    decoder->windows[j].visible=0;
    decoder->windows[j].memory_reserved=0;
    decoder->windows[j].is_empty=1;
    memset (decoder->windows[j].commands, 0,
        sizeof (decoder->windows[j].commands));
  }
  decoder->current_window=-1;
  clearTV(decoder);
  decoder->inited=1;
}

void cc708_reset(cc708_service_decoder *decoders)
{
  for (int i = 0; i<CCX_DECODERS_708_MAX_SERVICES; i++)
  {
    cc708_service_reset (&decoders[i]);
  }
  // Empty packet buffer
  clear_packet(&decoders[0]);
  decoders[0].parent->m_last_seq = -1;
}

int compWindowsPriorities (const void *a, const void *b)
{
  e708Window *w1=*(e708Window **)a;
  e708Window *w2=*(e708Window **)b;
  return w1->priority-w2->priority;
}

void clearTV (cc708_service_decoder *decoder)
{
  for (int i=0; i<I708_SCREENGRID_ROWS; i++)
  {
    memset (decoder->tv.chars[i], ' ', I708_SCREENGRID_COLUMNS);
    decoder->tv.chars[i][I708_SCREENGRID_COLUMNS]=0;
  }
};

void printTVtoBuf (cc708_service_decoder *decoder)
{
  int empty=1;
  decoder->textlen = 0;
  for (int i=0;i<75;i++)
  {
    for (int j=0;j<210;j++)
      if (decoder->tv.chars[i][j] != ' ')
      {
        empty=0;
        break;
      }
    if (!empty)
      break;
  }
  if (empty)
    return; // Nothing to write

  for (int i=0;i<75;i++)
  {
    int empty=1;
    for (int j=0;j<210;j++)
      if (decoder->tv.chars[i][j] != ' ')
        empty=0;
    if (!empty)
    {
      int f,l; // First,last used char
      for (f=0;f<210;f++)
        if (decoder->tv.chars[i][f] != ' ')
          break;
      for (l=209;l>0;l--)
        if (decoder->tv.chars[i][l]!=' ')
          break;
      for (int j=f;j<=l;j++)
        decoder->text[decoder->textlen++] = decoder->tv.chars[i][j];
      decoder->text[decoder->textlen++] = '\r';
      decoder->text[decoder->textlen++] = '\n';
    }
  }
  decoder->text[decoder->textlen++] = '\r';
  decoder->text[decoder->textlen++] = '\n';
  decoder->text[decoder->textlen++] = '\0';
}

void updateScreen (cc708_service_decoder *decoder)
{
  clearTV (decoder);

  // THIS FUNCTION WILL DO THE MAGIC OF ACTUALLY EXPORTING THE DECODER STATUS
  // TO SEVERAL FILES
  e708Window *wnd[I708_MAX_WINDOWS]; // We'll store here the visible windows that contain anything
  int visible=0;
  for (int i=0;i<I708_MAX_WINDOWS;i++)
  {
    if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
      wnd[visible++]=&decoder->windows[i];
  }
  qsort (wnd,visible,sizeof (e708Window *),compWindowsPriorities);

  for (int i=0;i<visible;i++)
  {
    int top,left;
    // For each window we calculate the top,left position depending on the
    // anchor
    switch (wnd[i]->anchor_point)
    {
    case anchorpoint_top_left:
      top=wnd[i]->anchor_vertical;
      left=wnd[i]->anchor_horizontal;
      break;
    case anchorpoint_top_center:
      top=wnd[i]->anchor_vertical;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
      break;
    case anchorpoint_top_right:
      top=wnd[i]->anchor_vertical;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
      break;
    case anchorpoint_middle_left:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
      left=wnd[i]->anchor_horizontal;
      break;
    case anchorpoint_middle_center:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
      break;
    case anchorpoint_middle_right:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count/2;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
      break;
    case anchorpoint_bottom_left:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count;
      left=wnd[i]->anchor_horizontal;
      break;
    case anchorpoint_bottom_center:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count/2;
      break;
    case anchorpoint_bottom_right:
      top=wnd[i]->anchor_vertical - wnd[i]->row_count;
      left=wnd[i]->anchor_horizontal - wnd[i]->col_count;
      break;
    default: // Shouldn't happen, but skip the window just in case
      continue;
    }
    if (top<0)
      top=0;
    if (left<0)
      left=0;
    int copyrows=top + wnd[i]->row_count >= I708_SCREENGRID_ROWS ?
        I708_SCREENGRID_ROWS - top : wnd[i]->row_count;
    int copycols=left + wnd[i]->col_count >= I708_SCREENGRID_COLUMNS ?
        I708_SCREENGRID_COLUMNS - left : wnd[i]->col_count;
    for (int j=0;j<copyrows;j++)
    {
      memcpy (decoder->tv.chars[top+j],wnd[i]->rows[j],copycols);
    }
  }
  printTVtoBuf(decoder);
  decoder->callback(decoder->service, decoder->userdata);
}

void rollupWindow(cc708_service_decoder *decoder, int window)
{
  for (int row=0; row<decoder->windows[window].row_count - 1; row++)
  {
    memcpy(decoder->windows[window].rows[row], decoder->windows[window].rows[row+1], decoder->windows[window].col_count);
  }
  memset(decoder->windows[window].rows[decoder->windows[window].row_count-1], ' ', decoder->windows[window].col_count);
}

/* This function handles future codes. While by definition we can't do any work on them, we must return
how many bytes would be consumed if these codes were supported, as defined in the specs.
Note: EXT1 not included */
// C2: Extended Miscellaneous Control Codes
// TODO: This code is completely untested due to lack of samples. Just following specs!
int handle_708_C2 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  if (data[0]<=0x07) // 00-07...
    return 1; // ... Single-byte control bytes (0 additional bytes)
  else if (data[0]<=0x0f) // 08-0F ...
    return 2; // ..two-byte control codes (1 additional byte)
  else if (data[0]<=0x17)  // 10-17 ...
    return 3; // ..three-byte control codes (2 additional bytes)
  return 4; // 18-1F => four-byte control codes (3 additional bytes)
}

int handle_708_C3 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  if (data[0]<0x80 || data[0]>0x9F)
    ;//ccx_common_logging.fatal_ftn (CCX_COMMON_EXIT_BUG_BUG, "Entry in handle_708_C3 with an out of range value.");
  if (data[0]<=0x87) // 80-87...
    return 5; // ... Five-byte control bytes (4 additional bytes)
  else if (data[0]<=0x8F) // 88-8F ...
    return 6; // ..Six-byte control codes (5 additional byte)
  // If here, then 90-9F ...

  // These are variable length commands, that can even span several segments
  // (they allow even downloading fonts or graphics).
  // TODO: Implemen if a sample ever appears
  return 0; // Unreachable, but otherwise there's compilers warnings
}

// This function handles extended codes (EXT1 + code), from the extended sets
// G2 (20-7F) => Mostly unmapped, except for a few characters.
// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708-B
// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
// TODO: This code is completely untested due to lack of samples. Just following specs!
// Returns number of used bytes, usually 1 (since EXT1 is not counted).
int handle_708_extended_char (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  int used;
  unsigned char c=0x20; // Default to space
  unsigned char code=data[0];
  if (/* data[i]>=0x00 && */ code<=0x1F) // Comment to silence warning
  {
    used=handle_708_C2 (decoder, data, data_length);
  }
  // Group G2 - Extended Miscellaneous Characters
  else if (code>=0x20 && code<=0x7F)
  {
    c=get_internal_from_G2 (code);
    used=1;
    process_character (decoder, c);
  }
  // Group C3
  else if (code>=0x80 && code<=0x9F)
  {
    used=handle_708_C3 (decoder, data, data_length);
    // TODO: Something
  }
  // Group G3
  else
  {
    c=get_internal_from_G3 (code);
    used=1;
    process_character (decoder, c);
  }
  return used;
}

void process_cr (cc708_service_decoder *decoder)
{
  switch (decoder->windows[decoder->current_window].attribs.print_dir)
  {
  case pd_left_to_right:
    decoder->windows[decoder->current_window].pen_column=0;
    if (decoder->windows[decoder->current_window].pen_row+1 < decoder->windows[decoder->current_window].row_count)
      decoder->windows[decoder->current_window].pen_row++;
    break;
  case pd_right_to_left:
    decoder->windows[decoder->current_window].pen_column=decoder->windows[decoder->current_window].col_count;
    if (decoder->windows[decoder->current_window].pen_row+1 < decoder->windows[decoder->current_window].row_count)
      decoder->windows[decoder->current_window].pen_row++;
    break;
  case pd_top_to_bottom:
    decoder->windows[decoder->current_window].pen_row=0;
    if (decoder->windows[decoder->current_window].pen_column+1 < decoder->windows[decoder->current_window].col_count)
      decoder->windows[decoder->current_window].pen_column++;
    break;
  case pd_bottom_to_top:
    decoder->windows[decoder->current_window].pen_row=decoder->windows[decoder->current_window].row_count;
    if (decoder->windows[decoder->current_window].pen_column+1 < decoder->windows[decoder->current_window].col_count)
      decoder->windows[decoder->current_window].pen_column++;
    break;
  }

  if (decoder->windows[decoder->current_window].anchor_point == anchorpoint_bottom_left ||
      decoder->windows[decoder->current_window].anchor_point == anchorpoint_bottom_center)
  {
    rollupWindow(decoder, decoder->current_window);
    updateScreen(decoder);
  }
}

int handle_708_C0 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  const char *name=COMMANDS_C0[data[0]];
  if (name==NULL)
    name="Reserved";
  int len=-1;
  // These commands have a known length even if they are reserved.
  if (/* data[0]>=0x00 && */ data[0]<=0xF) // Comment to silence warning
  {
    switch (data[0])
    {
    case 0x0d: //CR
      process_cr (decoder);
      break;
    case 0x0e: // HCR (Horizontal Carriage Return)
      // TODO: Process HDR
      break;
    case 0x0c: // FF (Form Feed)
      // TODO: Process FF
      break;
    }
    len=1;
  }
  else if (data[0]>=0x10 && data[0]<=0x17)
  {
    // Note that 0x10 is actually EXT1 and is dealt with somewhere else. Rest is undefined as per
    // CEA-708-D
    len=2;
  }
  else if (data[0]>=0x18 && data[0]<=0x1F)
  {
    // Only PE16 is defined.
    if (data[0]==0x18) // PE16
    {
      ; // TODO: Handle PE16
    }
    len=3;
  }
  if (len==-1)
  {
    return -1;
  }
  if (len>data_length)
  {
    return -1;
  }
  // TODO: Do something useful eventually
  return len;
}


void process_character (cc708_service_decoder *decoder, unsigned char internal_char)
{
  if (decoder->current_window==-1 ||
      !decoder->windows[decoder->current_window].is_defined) // Writing to a non existing window, skipping
    return;
  switch (internal_char)
  {
  default:
    decoder->windows[decoder->current_window].is_empty=0;
    decoder->windows[decoder->current_window].
      rows[decoder->windows[decoder->current_window].pen_row]
          [decoder->windows[decoder->current_window].pen_column]=internal_char;
    /* Not positive this interpretation is correct. Word wrapping is optional, so
                           let's assume we don't need to autoscroll */
    switch (decoder->windows[decoder->current_window].attribs.print_dir)
    {
    case pd_left_to_right:
      if (decoder->windows[decoder->current_window].pen_column+1 < decoder->windows[decoder->current_window].col_count)
        decoder->windows[decoder->current_window].pen_column++;
      break;
    case pd_right_to_left:
      if (decoder->windows->pen_column>0)
        decoder->windows[decoder->current_window].pen_column--;
      break;
    case pd_top_to_bottom:
      if (decoder->windows[decoder->current_window].pen_row+1 < decoder->windows[decoder->current_window].row_count)
        decoder->windows[decoder->current_window].pen_row++;
      break;
    case pd_bottom_to_top:
      if (decoder->windows[decoder->current_window].pen_row>0)
        decoder->windows[decoder->current_window].pen_row--;
      break;
    }
    break;
  }
}

// G0 - Code Set - ASCII printable characters
int handle_708_G0 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  // TODO: Substitution of the music note character for the ASCII DEL character
  unsigned char c=get_internal_from_G0 (data[0]);
  process_character (decoder, c);
  return 1;
}

// G1 Code Set - ISO 8859-1 LATIN-1 Character Set
int handle_708_G1 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  unsigned char c=get_internal_from_G1 (data[0]);
  process_character (decoder, c);
  return 1;
}

/*-------------------------------------------------------
                    WINDOW COMMANDS
  ------------------------------------------------------- */
void handle_708_CWx_SetCurrentWindow (cc708_service_decoder *decoder, int new_window)
{
  if (decoder->windows[new_window].is_defined)
    decoder->current_window=new_window;
}

void clearWindowText(e708Window *window)
{
  for (int i = 0; i<I708_MAX_ROWS; i++)
  {
    memset(window->rows[i], ' ', I708_MAX_COLUMNS);
    window->rows[i][I708_MAX_COLUMNS] = 0;
  }
  memset(window->rows[I708_MAX_ROWS], 0, I708_MAX_COLUMNS + 1);
  window->is_empty = 1;

}

void clearWindow (cc708_service_decoder *decoder, int window)
{
  if (decoder->windows[window].is_defined)
    clearWindowText(&decoder->windows[window]);
}

void handle_708_CLW_ClearWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
  if (windows_bitmap==0)
    ;//ccx_common_logging.debug_ftn(CCX_DMT_708, "None\n");
  else
  {
    for (int i=0; i<8; i++)
    {
      if (windows_bitmap & 1)
      {
        clearWindow (decoder, i);
      }
      windows_bitmap>>=1;
    }
  }
}

void handle_708_DSW_DisplayWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
  if (windows_bitmap==0)
    ;//ccx_common_logging.debug_ftn(CCX_DMT_708, "None\n");
  else
  {
    int changes=0;
    for (int i=0; i<8; i++)
    {
      if (windows_bitmap & 1)
      {
        if (!decoder->windows[i].visible)
        {
          changes=1;
          decoder->windows[i].visible=1;
        }
      }
      windows_bitmap>>=1;
    }
    if (changes)
      updateScreen (decoder);
  }
}

void handle_708_HDW_HideWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
  if (windows_bitmap==0)
    ;//ccx_common_logging.debug_ftn(CCX_DMT_708, "None\n");
  else
  {
    int changes=0;
    for (int i=0; i<8; i++)
    {
      if (windows_bitmap & 1)
      {
        if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
        {
          changes=1;
          decoder->windows[i].visible=0;
        }
        // TODO: Actually Hide Window
      }
      windows_bitmap>>=1;
    }
    if (changes)
      updateScreen (decoder);
  }
}

void handle_708_TGW_ToggleWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
  if (windows_bitmap==0)
    ;//ccx_common_logging.debug_ftn(CCX_DMT_708, "None\n");
  else
  {
    for (int i=0; i<8; i++)
    {
      if (windows_bitmap & 1)
      {
        decoder->windows[i].visible=!decoder->windows[i].visible;
      }
      windows_bitmap>>=1;
    }
    updateScreen(decoder);
  }
}

void handle_708_DFx_DefineWindow (cc708_service_decoder *decoder, int window, unsigned char *data)
{
  if (decoder->windows[window].is_defined &&
      memcmp (decoder->windows[window].commands, data+1, 6)==0)
  {
    return;
  }
  decoder->windows[window].number=window;
  int priority = (data[1]  ) & 0x7;
  int col_lock = (data[1]>>3) & 0x1;
  int row_lock = (data[1]>>4) & 0x1;
  int visible  = (data[1]>>5) & 0x1;
  int anchor_vertical = data[2] & 0x7f;
  int relative_pos = (data[2]>>7);
  int anchor_horizontal = data[3];
  int row_count = data[4] & 0xf;
  int anchor_point = data[4]>>4;
  int col_count = data[5] & 0x3f;
  int pen_style = data[6] & 0x7;
  int win_style = (data[6]>>3) & 0x7;
  col_count++; // These increments seems to be needed but no documentation
  row_count++; // backs it up

  if (anchor_vertical > I708_SCREENGRID_ROWS)
    anchor_vertical = I708_SCREENGRID_ROWS;

  decoder->windows[window].priority=priority;
  decoder->windows[window].col_lock=col_lock;
  decoder->windows[window].row_lock=row_lock;
  decoder->windows[window].visible=visible;
  decoder->windows[window].anchor_vertical=anchor_vertical;
  decoder->windows[window].relative_pos=relative_pos;
  decoder->windows[window].anchor_horizontal=anchor_horizontal;
  decoder->windows[window].row_count=row_count;
  decoder->windows[window].anchor_point=anchor_point;
  decoder->windows[window].col_count=col_count;
  decoder->windows[window].pen_style=pen_style;
  decoder->windows[window].win_style=win_style;
  if (!decoder->windows[window].is_defined)
  {
    // If the window is being created, all character positions in the window
    // are set to the fill color...
    // TODO: COLORS
    // ...and the pen location is set to (0,0)
    decoder->windows[window].pen_column=0;
    decoder->windows[window].pen_row=0;
    if (!decoder->windows[window].memory_reserved)
    {
      for (int i=0;i<=I708_MAX_ROWS;i++)
      {
        decoder->windows[window].rows[i]=(unsigned char *) malloc (I708_MAX_COLUMNS+1);
        if (decoder->windows[window].rows[i]==NULL) // Great
        {
          decoder->windows[window].is_defined=0;
          decoder->current_window=-1;
          for (int j=0;j<i;j++)
            free (decoder->windows[window].rows[j]);
          return; // TODO: Warn somehow
        }
      }
      decoder->windows[window].memory_reserved=1;
    }
    decoder->windows[window].is_defined=1;
    memset(&decoder->windows[window].attribs, 0, sizeof(e708Window_attribs));
    clearWindowText (&decoder->windows[window]);
  }

  // ...also makes the defined windows the current window (setCurrentWindow)
  handle_708_CWx_SetCurrentWindow (decoder, window);
  memcpy (decoder->windows[window].commands, data+1, 6);
}

void handle_708_SWA_SetWindowAttributes (cc708_service_decoder *decoder, unsigned char *data)
{
  int fill_color    = (data[1]   ) & 0x3f;
  int fill_opacity  = (data[1]>>6) & 0x03;
  int border_color  = (data[2]   ) & 0x3f;
  int border_type01 = (data[2]>>6) & 0x03;
  int justify       = (data[3]   ) & 0x03;
  int scroll_dir    = (data[3]>>2) & 0x03;
  int print_dir     = (data[3]>>4) & 0x03;
  int word_wrap     = (data[3]>>6) & 0x01;
  int border_type   = (data[3]>>5) | border_type01;
  int display_eff   = (data[4]   ) & 0x03;
  int effect_dir    = (data[4]>>2) & 0x03;
  int effect_speed  = (data[4]>>4) & 0x0f;
  if (decoder->current_window==-1)
  {
    // Can't do anything yet - we need a window to be defined first.
    return;
  }
  decoder->windows[decoder->current_window].attribs.fill_color=fill_color;
  decoder->windows[decoder->current_window].attribs.fill_opacity=fill_opacity;
  decoder->windows[decoder->current_window].attribs.border_color=border_color;
  decoder->windows[decoder->current_window].attribs.border_type01=border_type01;
  decoder->windows[decoder->current_window].attribs.justify=justify;
  decoder->windows[decoder->current_window].attribs.scroll_dir=scroll_dir;
  decoder->windows[decoder->current_window].attribs.print_dir=print_dir;
  decoder->windows[decoder->current_window].attribs.word_wrap=word_wrap;
  decoder->windows[decoder->current_window].attribs.border_type=border_type;
  decoder->windows[decoder->current_window].attribs.display_eff=display_eff;
  decoder->windows[decoder->current_window].attribs.effect_dir=effect_dir;
  decoder->windows[decoder->current_window].attribs.effect_speed=effect_speed;

}

void deleteWindow (cc708_service_decoder *decoder, int window)
{
  if (window==decoder->current_window)
  {
    // If the current window is deleted, then the decoder's current window ID
    // is unknown and must be reinitialized with either the SetCurrentWindow
    // or DefineWindow command.
    decoder->current_window=-1;
  }
  // TODO: Do the actual deletion (remove from display if needed, etc), mark as
  // not defined, etc
  if (decoder->windows[window].is_defined)
  {
    clearWindowText(&decoder->windows[window]);
  }
  decoder->windows[window].is_defined=0;
}

void handle_708_DLW_DeleteWindows (cc708_service_decoder *decoder, int windows_bitmap)
{
  int changes=0;
  if (windows_bitmap==0)
    ; //ccx_common_logging.debug_ftn(CCX_DMT_708, "None\n");
  else
  {
    for (int i=0; i<8; i++)
    {
      if (windows_bitmap & 1)
      {
        if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
          changes=1;
        deleteWindow (decoder, i);
      }
      windows_bitmap>>=1;
    }
  }
  if (changes)
    updateScreen (decoder);

}

/*-------------------------------------------------------
                    WINDOW COMMANDS
  ------------------------------------------------------- */
void handle_708_SPA_SetPenAttributes (cc708_service_decoder *decoder, unsigned char *data)
{
  int pen_size  = (data[1]   ) & 0x3;
  int offset    = (data[1]>>2) & 0x3;
  int text_tag  = (data[1]>>4) & 0xf;
  int font_tag  = (data[2]   ) & 0x7;
  int edge_type = (data[2]>>3) & 0x7;
  int underline = (data[2]>>4) & 0x1;
  int italic    = (data[2]>>5) & 0x1;
  if (decoder->current_window==-1)
  {
    // Can't do anything yet - we need a window to be defined first.
    return;
  }
  decoder->windows[decoder->current_window].pen.pen_size=pen_size;
  decoder->windows[decoder->current_window].pen.offset=offset;
  decoder->windows[decoder->current_window].pen.text_tag=text_tag;
  decoder->windows[decoder->current_window].pen.font_tag=font_tag;
  decoder->windows[decoder->current_window].pen.edge_type=edge_type;
  decoder->windows[decoder->current_window].pen.underline=underline;
  decoder->windows[decoder->current_window].pen.italic=italic;
}

void handle_708_SPC_SetPenColor (cc708_service_decoder *decoder, unsigned char *data)
{
  int fg_color   = (data[1]   ) & 0x3f;
  int fg_opacity = (data[1]>>6) & 0x03;
  int bg_color   = (data[2]   ) & 0x3f;
  int bg_opacity = (data[2]>>6) & 0x03;
  int edge_color = (data[3]>>6) & 0x3f;
  if (decoder->current_window==-1)
  {
    // Can't do anything yet - we need a window to be defined first.
    return;
  }

  decoder->windows[decoder->current_window].pen_color.fg_color=fg_color;
  decoder->windows[decoder->current_window].pen_color.fg_opacity=fg_opacity;
  decoder->windows[decoder->current_window].pen_color.bg_color=bg_color;
  decoder->windows[decoder->current_window].pen_color.bg_opacity=bg_opacity;
  decoder->windows[decoder->current_window].pen_color.edge_color=edge_color;
}


void handle_708_SPL_SetPenLocation (cc708_service_decoder *decoder, unsigned char *data)
{
  int row = data[1] & 0x0f;
  int col = data[2] & 0x3f;
  if (decoder->current_window==-1)
  {
    // Can't do anything yet - we need a window to be defined first.
    return;
  }
  decoder->windows[decoder->current_window].pen_row=row;
  decoder->windows[decoder->current_window].pen_column=col;
}


/*-------------------------------------------------------
                 SYNCHRONIZATION COMMANDS
  ------------------------------------------------------- */
void handle_708_DLY_Delay (cc708_service_decoder *decoder, int tenths_of_sec)
{
  // TODO: Probably ask for the current FTS and wait for this time before resuming -
  // not sure it's worth it though
}

void handle_708_DLC_DelayCancel (cc708_service_decoder *decoder)
{
  // TODO: See above
}

// C1 Code Set - Captioning Commands Control Codes
int handle_708_C1 (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  struct S_COMMANDS_C1 com=COMMANDS_C1[data[0]-0x80];
  if (com.length>data_length)
  {
    return -1;
  }
  switch (com.code)
  {
  case CW0: /* SetCurrentWindow */
  case CW1:
  case CW2:
  case CW3:
  case CW4:
  case CW5:
  case CW6:
  case CW7:
    handle_708_CWx_SetCurrentWindow (decoder, com.code-CW0); /* Window 0 to 7 */
    break;
  case CLW:
    handle_708_CLW_ClearWindows (decoder, data[1]);
    break;
  case DSW:
    handle_708_DSW_DisplayWindows (decoder, data[1]);
    break;
  case HDW:
    handle_708_HDW_HideWindows (decoder, data[1]);
    break;
  case TGW:
    handle_708_TGW_ToggleWindows (decoder, data[1]);
    break;
  case DLW:
    handle_708_DLW_DeleteWindows (decoder, data[1]);
    break;
  case DLY:
    handle_708_DLY_Delay (decoder, data[1]);
    break;
  case DLC:
    handle_708_DLC_DelayCancel (decoder);
    break;
  case RST:
    cc708_service_reset(decoder);
    break;
  case SPA:
    handle_708_SPA_SetPenAttributes (decoder, data);
    break;
  case SPC:
    handle_708_SPC_SetPenColor (decoder, data);
    break;
  case SPL:
    handle_708_SPL_SetPenLocation (decoder, data);
    break;
  case RSV93:
  case RSV94:
  case RSV95:
  case RSV96:
    break;
  case SWA:
    handle_708_SWA_SetWindowAttributes (decoder, data);
    break;
  case DF0:
  case DF1:
  case DF2:
  case DF3:
  case DF4:
  case DF5:
  case DF6:
  case DF7:
    handle_708_DFx_DefineWindow (decoder, com.code-DF0, data); /* Window 0 to 7 */
    break;
  default:
    break;
  }

  return com.length;
}


void process_service_block (cc708_service_decoder *decoder, unsigned char *data, int data_length)
{
  int i=0;
  while (i<data_length)
  {
    int used=-1;
    if (data[i]!=EXT1)
    {
      // Group C0
      if (/* data[i]>=0x00 && */ data[i]<=0x1F) // Comment to silence warning
      {
        used=handle_708_C0 (decoder,data+i,data_length-i);
      }
      // Group G0
      else if (data[i]>=0x20 && data[i]<=0x7F)
      {
        used=handle_708_G0 (decoder,data+i,data_length-i);
      }
      // Group C1
      else if (data[i]>=0x80 && data[i]<=0x9F)
      {
        used=handle_708_C1 (decoder,data+i,data_length-i);
      }
      // Group C2
      else
        used=handle_708_G1 (decoder,data+i,data_length-i);
      if (used==-1)
      {
        // TODO: Not sure if a local reset is going to be helpful here.
        cc708_service_reset (decoder);
        return;
      }
    }
    else // Use extended set
    {
      used=handle_708_extended_char (decoder, data+i+1,data_length-1);
      used++; // Since we had EXT1
    }
    i+=used;
  }

  // update rollup windows
  int update = 0;
  for (int i = 0; i<I708_MAX_WINDOWS; i++)
  {
    if (decoder->windows[i].is_defined && decoder->windows[i].visible &&
      (decoder->windows[i].anchor_point == anchorpoint_bottom_left ||
      decoder->windows[i].anchor_point == anchorpoint_bottom_center))
    {
      update++;
      break;
    }
  }
  if (update)
  {
    updateScreen(decoder);
  }
}

void process_current_packet (cc708_service_decoder *decoders)
{
  int seq = (decoders[0].parent->m_current_packet[0] & 0xC0) >> 6; // Two most significants bits
  int len = decoders[0].parent->m_current_packet[0] & 0x3F; // 6 least significants bits
  if (decoders[0].parent->m_current_packet_length == 0)
    return;

  if (len==0) // This is well defined in EIA-708; no magic.
    len=128;
  else
    len=len*2;
  // Note that len here is the length including the header
  if (decoders[0].parent->m_current_packet_length != len) // Is this possible?
  {
    cc708_reset(decoders);
    return;
  }
  int last_seq = decoders[0].parent->m_last_seq;
  if ((last_seq != -1) && ((last_seq+1)%4 != seq))
  {
    cc708_reset(decoders);
    return;
  }
  decoders[0].parent->m_last_seq = seq;

  unsigned char *pos = decoders[0].parent->m_current_packet + 1;

  while (pos < decoders[0].parent->m_current_packet + len)
  {
    int service_number=(pos[0] & 0xE0)>>5; // 3 more significant bits
    int block_length = (pos[0] & 0x1F); // 5 less significant bits

    if (service_number==7) // There is an extended header
    {
      pos++;
      service_number=(pos[0] & 0x3F); // 6 more significant bits
      if (service_number<7)
      {
      }
      pos = decoders[0].parent->m_current_packet + len;
      break;
    }

    pos++; // Move to service data
    if (service_number==0 && block_length!=0) // Illegal, but specs say what to do...
    {
      pos = decoders[0].parent->m_current_packet + len; // Move to end
      break;
    }

    if (service_number>0 && decoders[service_number].inited)
      process_service_block (&decoders[service_number], pos, block_length);

    pos+=block_length; // Skip data
  }

  clear_packet(&decoders[0]);

  if (pos != decoders[0].parent->m_current_packet + len) // For some reason we didn't parse the whole packet
  {
    cc708_reset(decoders);
  }

  if (len<128 && *pos) // Null header is mandatory if there is room
  {
    ;//ccx_common_logging.debug_ftn(CCX_DMT_708, "Warning: Null header expected but not found.\n");
  }
}

void decode_708 (const unsigned char *data, int datalength, cc708_service_decoder* decoders)
{
  /* Note: The data has this format:
        1 byte for cc_valid and cc_type
        2 bytes for the actual data */
  for (int i=0; i<datalength; i+=3)
  {
    unsigned char cc_valid=data[i] & 0x04;
    unsigned char cc_type=data[i] & 0x03;

    switch (cc_type)
    {
    case 0:
      // only use 608 as fallback
      if (!decoders[0].parent->m_seen708)
        decode_cc(decoders[0].parent->m_cc608decoder, (uint8_t*)data+i, 3);
      break;
    case 2:
      if (cc_valid==0) // This ends the previous packet
        process_current_packet(decoders);
      else
      {
        if (decoders[0].parent->m_current_packet_length < 254)
        {
          decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++]=data[i+1];
          decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++]=data[i+2];
        }
      }
      break;
    case 3:
      process_current_packet(decoders);
      if (cc_valid)
      {
        if (decoders[0].parent->m_current_packet_length < 128)
        {
          decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++]=data[i+1];
          decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++]=data[i+2];
        }
      }
      break;
    default:
      break;
    }
  }
}

void ccx_decoders_708_init(cc708_service_decoder *decoders, void (*handler)(int service, void *userdata), void *userdata, CDecoderCC708 *parent)
{
  for (int i = 0; i<CCX_DECODERS_708_MAX_SERVICES; i++)
  {
    cc708_service_reset (&decoders[i]);
    decoders[i].srt_counter=0;
    decoders[i].service = i;
    decoders[i].callback = handler;
    decoders[i].userdata = userdata;
    decoders[i].parent = parent;
  }
  decoders[0].parent->m_cc608decoder->callback = handler;
  decoders[0].parent->m_cc608decoder->userdata = userdata;

  decoders[0].parent->m_current_packet_length = 0;
  decoders[0].parent->m_last_seq = -1;
  decoders[0].parent->m_seen708 = false;
  decoders[0].parent->m_seen608 = false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CDecoderCC708::CDecoderCC708()
{
  m_inited = false;
  cc_decoder_init();
}

CDecoderCC708::~CDecoderCC708()
{
  delete [] m_cc708decoders;
  cc_decoder_close(m_cc608decoder);
}

void CDecoderCC708::Init(void (*handler)(int service, void *userdata), void *userdata)
{
  m_cc608decoder = cc_decoder_open();
  m_cc708decoders = new cc708_service_decoder[8];
  ccx_decoders_708_init(m_cc708decoders, handler, userdata, this);
}

void CDecoderCC708::Decode(const unsigned char *data, int datalength)
{
  decode_708(data, datalength, m_cc708decoders);
}
