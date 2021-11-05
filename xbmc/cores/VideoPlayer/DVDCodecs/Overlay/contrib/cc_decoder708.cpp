/*
 * this is mostly borrowed from ccextractor http://ccextractor.sourceforge.net/
 */

#include "cc_decoder708.h"

#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <stdio.h>
#include <stdlib.h>
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

//! @todo Probably not right
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

//! @todo Probably not right
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

struct commandC1 commandsC1[32] = {{CommandCodeC1::CW0, "CW0", "SetCurrentWindow0", 1},
                                   {CommandCodeC1::CW1, "CW1", "SetCurrentWindow1", 1},
                                   {CommandCodeC1::CW2, "CW2", "SetCurrentWindow2", 1},
                                   {CommandCodeC1::CW3, "CW3", "SetCurrentWindow3", 1},
                                   {CommandCodeC1::CW4, "CW4", "SetCurrentWindow4", 1},
                                   {CommandCodeC1::CW5, "CW5", "SetCurrentWindow5", 1},
                                   {CommandCodeC1::CW6, "CW6", "SetCurrentWindow6", 1},
                                   {CommandCodeC1::CW7, "CW7", "SetCurrentWindow7", 1},
                                   {CommandCodeC1::CLW, "CLW", "ClearWindows", 2},
                                   {CommandCodeC1::DSW, "DSW", "DisplayWindows", 2},
                                   {CommandCodeC1::HDW, "HDW", "HideWindows", 2},
                                   {CommandCodeC1::TGW, "TGW", "ToggleWindows", 2},
                                   {CommandCodeC1::DLW, "DLW", "DeleteWindows", 2},
                                   {CommandCodeC1::DLY, "DLY", "Delay", 2},
                                   {CommandCodeC1::DLC, "DLC", "DelayCancel", 1},
                                   {CommandCodeC1::RST, "RST", "Reset", 1},
                                   {CommandCodeC1::SPA, "SPA", "SetPenAttributes", 3},
                                   {CommandCodeC1::SPC, "SPC", "SetPenColor", 4},
                                   {CommandCodeC1::SPL, "SPL", "SetPenLocation", 3},
                                   {CommandCodeC1::RSV93, "RSV93", "Reserved", 1},
                                   {CommandCodeC1::RSV94, "RSV94", "Reserved", 1},
                                   {CommandCodeC1::RSV95, "RSV95", "Reserved", 1},
                                   {CommandCodeC1::RSV96, "RSV96", "Reserved", 1},
                                   {CommandCodeC1::SWA, "SWA", "SetWindowAttributes", 5},
                                   {CommandCodeC1::DF0, "DF0", "DefineWindow0", 7},
                                   {CommandCodeC1::DF1, "DF0", "DefineWindow1", 7},
                                   {CommandCodeC1::DF2, "DF0", "DefineWindow2", 7},
                                   {CommandCodeC1::DF3, "DF0", "DefineWindow3", 7},
                                   {CommandCodeC1::DF4, "DF0", "DefineWindow4", 7},
                                   {CommandCodeC1::DF5, "DF0", "DefineWindow5", 7},
                                   {CommandCodeC1::DF6, "DF0", "DefineWindow6", 7},
                                   {CommandCodeC1::DF7, "DF0", "DefineWindow7", 7}};

//------------------------- DEFAULT AND PREDEFINED -----------------------------

e708Pen_color defaultPenColor = {0x3f, 0, 0, 0, 0};

e708Pen_attribs defaultPenAttribs = {PenSize::STANDARD,
                                     0,
                                     PenTextTag::UNDEFINED_12,
                                     PenFontStyle::DEFAULT_OR_UNDEFINED,
                                     PenEdgeType::NONE,
                                     0,
                                     0};

struct e708Window_attribs predefinitedWindowStyle[] = {{
    WindowJustify::LEFT, WindowPrintDirection::LEFT_TO_RIGHT,
    WindowScrollDirection::BOTTOM_TO_TOP, 0, WindowScrollDisplayEffect::SNAP,
    WindowEffectDirection::LEFT_TO_RIGHT, //n/a
    0, //n/a
    0, WindowFillOpacity::FO_SOLID, WindowBorderType::NONE,
    0 //n/a
}};

//---------------------------------- HELPERS ------------------------------------

void clear_packet(cc708_service_decoder *decoder)
{
  decoder->parent->m_current_packet_length = 0;
  decoder->parent->m_is_current_packet_header_parsed = false;
  memset(decoder->parent->m_current_packet, 0, CC708_MAX_PACKET_LENGTH * sizeof(unsigned char));
}

void clearTV(cc708_service_decoder* decoder)
{
  for (int i = 0; i < I708_SCREENGRID_ROWS; i++)
  {
    memset(decoder->tv.chars[i], 0, I708_SCREENGRID_COLUMNS * sizeof(e708_symbol));
  }
};

int DecoderHasVisibleWindows(cc708_service_decoder* decoder)
{
  for (int i = 0; i < I708_MAX_WINDOWS; i++)
  {
    if (decoder->windows[i].visible)
      return 1;
  }
  return 0;
}

void WindowClearRow(e708Window& window, int rowIndex)
{
  if (window.memory_reserved)
  {
    memset(window.rows[rowIndex], 0, I708_MAX_COLUMNS * sizeof(e708_symbol));
    window.pen_attribs[rowIndex] = defaultPenAttribs;
    window.pen_colors[rowIndex] = defaultPenColor;
  }
}

void WindowClearText(e708Window& window)
{
  for (int i = 0; i < I708_MAX_ROWS; i++)
  {
    WindowClearRow(window, i);
  }
  window.is_empty = 1;
}

void clearWindow(cc708_service_decoder* decoder, int windowId)
{
  if (decoder->windows[windowId].is_defined)
    WindowClearText(decoder->windows[windowId]);
  //OPT fill window with a window fill color
}

void WindowApplyStyle(e708Window& window, e708Window_attribs& style)
{
    window.attribs.border_color = style.border_color;
	window.attribs.border_type = style.border_type;
	window.attribs.display_effect = style.display_effect;
	window.attribs.effect_direction = style.effect_direction;
	window.attribs.effect_speed = style.effect_speed;
	window.attribs.fill_color = style.fill_color;
	window.attribs.fill_opacity = style.fill_opacity;
	window.attribs.justify = style.justify;
	window.attribs.print_direction = style.print_direction;
	window.attribs.scroll_direction = style.scroll_direction;
	window.attribs.word_wrap = style.word_wrap;
}

void ServiceReset(cc708_service_decoder* decoder)
{
  // There's lots of other stuff that we need to do, such as canceling delays
  for (e708Window& window : decoder->windows)
  {
    WindowClearText(window);
    window.is_defined = 0;
    window.visible = 0;
    window.memory_reserved = 0;
    window.is_empty = 1;
    memset(window.commands, 0, sizeof(window.commands));
  }
  decoder->current_window = -1;
  clearTV(decoder);
}

void DecodersReset(cc708_service_decoder* decoders)
{
  CLog::Log(LOGDEBUG, "{} - Resetting all decoders", __FUNCTION__);
  for (int i = 0; i < CC708_MAX_SERVICES; i++)
  {
    if (!decoders->parent->m_servicesActive[i])
      continue;
    
    ServiceReset(&decoders[i]);
  }
  // Empty packet buffer
  clear_packet(&decoders[0]);
  decoders[0].parent->m_last_sequence = CC708_NO_LAST_SEQUENCE;
}

int CompareWindowsPriorities(const void* a, const void* b)
{
  const e708Window* w1 = *(e708Window* const*)a;
  const e708Window* w2 = *(e708Window* const*)b;
  return w1->priority - w2->priority;
}

int IsRowEmpty(tvscreen& tv, int rowIndex)
{
  for (int j = 0; j < I708_SCREENGRID_COLUMNS; j++)
  {
    if (CCX_DTVCC_SYM_IS_SET(tv.chars[rowIndex][j]))
      return 0;
  }
  return 1;
}

int IsScreenEmpty(tvscreen& tv)
{
  for (int i = 0; i < I708_SCREENGRID_COLUMNS; i++)
  {
    if (!IsRowEmpty(tv, i))
      return 0;
  }
  return 1;
}

std::string PenColorToColorHexTag(int colorInt)
{
  unsigned int red = colorInt >> 4;
  unsigned int green = (colorInt >> 2) & 0x3;
  unsigned int blue = colorInt & 0x3;
  red = (255 / 3) * red;
  green = (255 / 3) * green;
  blue = (255 / 3) * blue;
  auto color = UTILS::COLOR::ConvertIntToRGB(blue, green, red);
  return StringUtils::Format("{{\\c&H{:06x}&}}", color);
}

void WriteTagOpen(cc708_service_decoder* decoder, int rowIndex)
{
  tvscreen& tv = decoder->tv;

  if (tv.pen_attribs[rowIndex].italic)
  {
    decoder->textlen += sprintf(decoder->text + decoder->textlen, "{\\i1}");
  }
  if (tv.pen_attribs[rowIndex].underline)
  {
    decoder->textlen += sprintf(decoder->text + decoder->textlen, "{\\u1}");
  }
  if (tv.pen_colors[rowIndex].fg_color != 0x3f) // assuming white is default
  {
    std::string colorTag = PenColorToColorHexTag(tv.pen_colors[rowIndex].fg_color);
    decoder->textlen += sprintf(decoder->text + decoder->textlen, colorTag.c_str());
  }
}

void WriteTagClose(cc708_service_decoder* decoder, int rowIndex)
{
  tvscreen& tv = decoder->tv;
  // NOTE: Each "if" is in reversed order compared to WriteTagOpen
  if (tv.pen_colors[rowIndex].fg_color != 0x3f) // assuming white is default
  {
    decoder->textlen += sprintf(decoder->text + decoder->textlen, "{\\c}");
  }
  if (tv.pen_attribs[rowIndex].underline)
  {
    decoder->textlen += sprintf(decoder->text + decoder->textlen, "{\\u0}");
  }
  if (tv.pen_attribs[rowIndex].italic)
  {
    decoder->textlen += sprintf(decoder->text + decoder->textlen, "{\\i0}");
  }
}

void WriteRow(cc708_service_decoder* decoder, int rowIndex)
{
  tvscreen& tv = decoder->tv;
  // Get position of the first/last char set
  int firstChar = 0;
  int lastChar = 0;
  for (; firstChar < I708_SCREENGRID_COLUMNS; firstChar++)
  {
  	if (CCX_DTVCC_SYM_IS_SET(tv.chars[rowIndex][firstChar]))
			break;
  }
  for (lastChar = I708_SCREENGRID_COLUMNS - 1; lastChar > 0; lastChar--)
  {
  	if (CCX_DTVCC_SYM_IS_SET(tv.chars[rowIndex][lastChar]))
			break;
  }
  
  // Convert symbols to chars and set them to demuxer text buffer
  for (int j = firstChar; j <= lastChar; j++)
  {
    if (CCX_DTVCC_SYM_IS_16(tv.chars[rowIndex][j]))
    {
      decoder->text[decoder->textlen++] = CCX_DTVCC_SYM_16_FIRST(tv.chars[rowIndex][j]);
      decoder->text[decoder->textlen++] = CCX_DTVCC_SYM_16_SECOND(tv.chars[rowIndex][j]);
    }
    else
    {
      decoder->text[decoder->textlen++] = CCX_DTVCC_SYM(tv.chars[rowIndex][j]);
    }
  }
}

void printTVtoBuf(cc708_service_decoder* decoder)
{
  // Convert tv data to subtitle
  decoder->textlen = 0;
  tvscreen& tv = decoder->tv;
  if (IsScreenEmpty(tv))
      return;
  
  //! @todo Missing implementation of timing+delay?
  //! maybe is needed a rework in similar way of upstream SRT conversion
 
  for (int i = 0; i < I708_SCREENGRID_ROWS; i++)
  {
    if (!IsRowEmpty(tv, i))
    {
      WriteTagOpen(decoder, i);
      WriteRow(decoder, i);
      WriteTagClose(decoder, i);
    }
  }


	


  /*
  for (unsigned char(&row)[I708_SCREENGRID_COLUMNS] : decoder->tv.chars)
  {
    for (int j = 0; j < I708_SCREENGRID_COLUMNS; j++)
      if (row[j] != ' ')
      {
        empty = 0;
        break;
      }
    if (!empty)
      break;
  }
  if (empty)
    return; // Nothing to write

  for (unsigned char(&row)[I708_SCREENGRID_COLUMNS] : decoder->tv.chars)
  {
    int empty = 1;
    for (int j = 0; j < I708_SCREENGRID_COLUMNS; j++)
      if (row[j] != ' ')
        empty = 0;
    if (!empty)
    {
      int f, l; // First,last used char
      for (f = 0; f < I708_SCREENGRID_COLUMNS; f++)
        if (row[f] != ' ')
          break;
      for (l = I708_SCREENGRID_COLUMNS - 1; l > 0; l--)
        if (row[l] != ' ')
          break;
      for (int j = f; j <= l; j++)
        decoder->text[decoder->textlen++] = row[j];
      decoder->text[decoder->textlen++] = '\r';
      decoder->text[decoder->textlen++] = '\n';
    }
  }

  // FIXME: the end-of-string char is often wrong cause unexpected behaviours
  if (decoder->textlen >= 2)
  {
    if (decoder->text[decoder->textlen - 2] == '\r' &&
        decoder->text[decoder->textlen - 1] == '\n' && decoder->text[decoder->textlen] != '\0')
    {
      decoder->text[decoder->textlen] = '\0';
    }
    else if (decoder->text[decoder->textlen] != '\0')
    {
      decoder->text[decoder->textlen++] = '\r';
      decoder->text[decoder->textlen++] = '\n';
      decoder->text[decoder->textlen++] = '\0';
    }
  }
  */
}

void updateScreen(cc708_service_decoder* decoder)
{
  clearTV(decoder);

  // THIS FUNCTION WILL DO THE MAGIC OF ACTUALLY EXPORTING THE DECODER STATUS
  // TO SEVERAL FILES
  e708Window* wnd[I708_MAX_WINDOWS]; // We'll store here the visible windows that contain anything
  int visible = 0;
  for (e708Window& window : decoder->windows)
  {
    if (window.is_defined && window.visible && !window.is_empty)
      wnd[visible++] = &window;
  }

  // Use priorities to solve windows overlap
  qsort(wnd, visible, sizeof(e708Window*), CompareWindowsPriorities);

  for (int i = 0; i < visible; i++)
  {
    int top, left;
    // For each window we calculate the top,left position depending on the
    // anchor
    switch (wnd[i]->anchor_point)
    {
      case PenAnchorPoint::TOP_LEFT:
        top = wnd[i]->anchor_vertical;
        left = wnd[i]->anchor_horizontal;
        break;
      case PenAnchorPoint::TOP_CENTER:
        top = wnd[i]->anchor_vertical;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
        break;
      case PenAnchorPoint::TOP_RIGHT:
        top = wnd[i]->anchor_vertical;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
        break;
      case PenAnchorPoint::MIDDLE_LEFT:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
        left = wnd[i]->anchor_horizontal;
        break;
      case PenAnchorPoint::MIDDLE_CENTER:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
        break;
      case PenAnchorPoint::MIDDLE_RIGHT:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
        break;
      case PenAnchorPoint::BOTTOM_LEFT:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count;
        left = wnd[i]->anchor_horizontal;
        break;
      case PenAnchorPoint::BOTTOM_CENTER:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
        break;
      case PenAnchorPoint::BOTTOM_RIGHT:
        top = wnd[i]->anchor_vertical - wnd[i]->row_count;
        left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
        break;
      default: // Shouldn't happen, but skip the window just in case
        continue;
    }
    if (top < 0)
      top = 0;
    if (left < 0)
      left = 0;
    int copyrows = top + wnd[i]->row_count >= I708_SCREENGRID_ROWS ? I708_SCREENGRID_ROWS - top
                                                                   : wnd[i]->row_count;
    int copycols = left + wnd[i]->col_count >= I708_SCREENGRID_COLUMNS
                       ? I708_SCREENGRID_COLUMNS - left
                       : wnd[i]->col_count;
    for (int j = 0; j < copyrows; j++)
    {
      memcpy(decoder->tv.chars[top + j], wnd[i]->rows[j], copycols * sizeof(e708_symbol));
      decoder->tv.pen_attribs[top + j] = wnd[i]->pen_attribs[j];
      decoder->tv.pen_colors[top + j] = wnd[i]->pen_colors[j];
    }
  }
  printTVtoBuf(decoder);
  decoder->callback(decoder->tv.service_number, decoder->userdata);
}

void process_hcr(cc708_service_decoder* decoder)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }

  e708Window& window = decoder->windows[decoder->current_window];
  window.pen_column = 0;
  WindowClearRow(window, window.pen_row);
}

void process_ff(cc708_service_decoder* decoder)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }
  e708Window& window = decoder->windows[decoder->current_window];
  window.pen_column = 0;
  window.pen_row = 0;
  //CEA-708-D doesn't say we have to clear neither window text nor text line,
  //but it seems we have to clean the line
  //_dtvcc_window_clear_text(window);
}

void process_etx(cc708_service_decoder *decoder)
{
	//it can help decoders with screen output, but could it help us?
}

void WindowRollup(cc708_service_decoder* decoder, int windowIndx)
{
  e708Window& window = decoder->windows[windowIndx];

  for (int i = 0; i < window.row_count - 1; i++)
  {
    memcpy(window.rows[i], window.rows[i + 1], I708_MAX_COLUMNS * sizeof(e708_symbol));
    window.pen_colors[i] = window.pen_colors[i + 1];
    window.pen_attribs[i] = window.pen_attribs[i + 1];
  }

  WindowClearRow(window, window.row_count - 1);
}

void process_cr (cc708_service_decoder *decoder)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }

  e708Window& window = decoder->windows[decoder->current_window];

  int rollupRequired = 0;
  switch (window.attribs.print_direction)
  {
    case WindowPrintDirection::LEFT_TO_RIGHT:
      window.pen_column = 0;
      if (window.pen_row + 1 < window.row_count)
        window.pen_row++;
      else
        rollupRequired = 1;
      break;
    case WindowPrintDirection::RIGHT_TO_LEFT:
      window.pen_column = window.col_count;
      if (window.pen_row + 1 < window.row_count)
        window.pen_row++;
      else
        rollupRequired = 1;
      break;
    case WindowPrintDirection::TOP_TO_BOTTOM:
      window.pen_row = 0;
      if (window.pen_column + 1 < window.col_count)
        window.pen_column++;
      else
        rollupRequired = 1;
      break;
    case WindowPrintDirection::BOTTOM_TO_TOP:
      window.pen_row = window.row_count;
      if (window.pen_column + 1 < window.col_count)
        window.pen_column++;
      else
        rollupRequired = 1;
      break;
    default:
      CLog::Log(LOGERROR, "{} - Unhandled branch", __FUNCTION__);
      break;
  }

  if (window.is_defined)
  {
    CLog::Log(LOGDEBUG, "{} - Rolling up", __FUNCTION__);
    updateScreen(decoder);

    if (rollupRequired)
    {
      if (decoder->parent->m_no_rollup)
        WindowClearRow(window, window.pen_row);
      else
        WindowRollup(decoder, decoder->current_window);
    }
  }
  /*
  if (window.anchor_point == PenAnchorPoint::BOTTOM_LEFT ||
      window.anchor_point == PenAnchorPoint::BOTTOM_CENTER)
  {
    WindowRollup(decoder, decoder->current_window);
    updateScreen(decoder);
  }
  */
}

void process_character(cc708_service_decoder* decoder, e708_symbol symbol)
{
  int windowId = decoder->current_window;
  if (windowId == -1 || !decoder->windows[windowId].is_defined)
  {
    CLog::Log(LOGERROR, "{} - Writing to a non existing window, skipping", __FUNCTION__);
    return;
  }

  e708Window& window = decoder->windows[windowId];

  window.is_empty = 0;
  window.rows[window.pen_row][window.pen_column] = symbol;
  switch (window.attribs.print_direction)
  {
    case WindowPrintDirection::LEFT_TO_RIGHT:
      if (window.pen_column + 1 < window.col_count)
        window.pen_column++;
      break;
    case WindowPrintDirection::RIGHT_TO_LEFT:
      if (decoder->windows->pen_column > 0)
        window.pen_column--;
      break;
    case WindowPrintDirection::TOP_TO_BOTTOM:
      if (window.pen_row + 1 < window.row_count)
        window.pen_row++;
      break;
    case WindowPrintDirection::BOTTOM_TO_TOP:
      if (window.pen_row > 0)
        window.pen_row--;
      break;
    default:
      CLog::Log(LOGERROR, "{} - Unhandled branch", __FUNCTION__);
      break;
  }
}

//---------------------------------- COMMANDS ------------------------------------

void handle_708_CWx_SetCurrentWindow(cc708_service_decoder* decoder, int windowId)
{
  if (decoder->windows[windowId].is_defined)
    decoder->current_window = windowId;
  else
    CLog::Log(LOGERROR, "{} - Window {} is not defined", __FUNCTION__, windowId);
}

void handle_708_CLW_ClearWindows(cc708_service_decoder* decoder, int windowId)
{
  if (windowId == 0)
    CLog::Log(LOGDEBUG, "{} - No window ID", __FUNCTION__);
  else
  {
    for (int i = 0; i < I708_MAX_WINDOWS; i++)
    {
      if (windowId & 1)
      {
        clearWindow(decoder, i);
      }
      windowId >>= 1;
    }
  }
}

void handle_708_DSW_DisplayWindows(cc708_service_decoder* decoder, int windowId)
{
  if (windowId == 0)
    CLog::Log(LOGDEBUG, "{} - No window ID", __FUNCTION__);
  else
  {
    int changes = 0;
    for (e708Window& window : decoder->windows)
    {
      if (windowId & 1)
      {
        if (!window.visible)
        {
          changes = 1;
          window.visible = 1;
        }
      }
      windowId >>= 1;
    }
    if (changes)
      updateScreen(decoder);
  }
}

void handle_708_HDW_HideWindows(cc708_service_decoder* decoder, int windowId)
{
  if (windowId == 0)
    CLog::Log(LOGDEBUG, "{} - No window ID", __FUNCTION__);
  else
  {
    int changes = 0;
    for (e708Window& window : decoder->windows)
    {
      if (windowId & 1)
      {
        if (window.is_defined && window.visible && !window.is_empty)
        {
          changes = 1;
          window.visible = 0;
        }
        //! @todo Actually Hide Window
      }
      windowId >>= 1;
    }
    if (changes)
      updateScreen(decoder);
  }
}

void handle_708_TGW_ToggleWindows(cc708_service_decoder* decoder, int windowId)
{
  if (windowId == 0)
    CLog::Log(LOGDEBUG, "{} - No window ID", __FUNCTION__);
  else
  {
    for (e708Window& window : decoder->windows)
    {
      if (windowId & 1)
      {
        window.visible = window.visible == 1 ? 0 : 1;
      }
      windowId >>= 1;
    }
    updateScreen(decoder);
  }
}

void handle_708_DFx_DefineWindow(cc708_service_decoder* decoder, int windowId, unsigned char* data)
{
  e708Window& window = decoder->windows[windowId];
  if (window.is_defined && memcmp(window.commands, data + 1, 6) == 0)
  {
    // When a decoder receives a DefineWindow command for an existing window, the
    // command is to be ignored if the command parameters are unchanged from the
    // previous window definition.
    CLog::Log(LOGDEBUG, "{} - Repeated window definition, ignored", __FUNCTION__);
    return;
  }

  window.number = windowId;
  int priority = (data[1]) & 0x7;
  int col_lock = (data[1] >> 3) & 0x1;
  int row_lock = (data[1] >> 4) & 0x1;
  int visible = (data[1] >> 5) & 0x1;
  int anchor_vertical = data[2] & 0x7f;
  int relative_pos = (data[2] >> 7);
  int anchor_horizontal = data[3];
  int row_count = (data[4] & 0xf) + 1; //according to CEA-708-D
  int anchor_point = data[4] >> 4;
  int col_count = (data[5] & 0x3f); //according to CEA-708-D
  int pen_style = data[6] & 0x7;
  int win_style = (data[6] >> 3) & 0x7;
  col_count++; // These increments seems to be needed but no documentation
  row_count++; // backs it up

  /*
   * Korean samples have "anchor_vertical" and "anchor_horizontal" mixed up,
   * this seems to be an encoder issue, but we can workaround it
   */
  if (anchor_vertical > I708_SCREENGRID_ROWS - row_count)
    anchor_vertical = I708_SCREENGRID_ROWS - row_count;
  if (anchor_horizontal > I708_SCREENGRID_COLUMNS - col_count)
    anchor_horizontal = I708_SCREENGRID_COLUMNS - col_count;

  window.priority = priority;
  window.col_lock = col_lock;
  window.row_lock = row_lock;
  window.visible = visible;
  window.anchor_vertical = anchor_vertical;
  window.relative_pos = relative_pos;
  window.anchor_horizontal = anchor_horizontal;
  window.row_count = row_count;
  window.anchor_point = static_cast<PenAnchorPoint>(anchor_point);
  window.col_count = col_count;
  window.pen_style = pen_style;
  window.win_style = win_style;

  if (win_style == 0)
  {
    window.win_style = 1;
  }
  //! @todo apply static win_style preset
  //! @todo apply static pen_style preset
  if (!window.is_defined)
  {
    // If the window is being created, all character positions in the window
    // are set to the fill color and the pen location is set to (0,0)
    //! @todo COLORS
    window.pen_column = 0;
    window.pen_row = 0;
    if (!window.memory_reserved)
    {
      for (int i = 0; i <= I708_MAX_ROWS; i++)
      {
        window.rows[i] = (e708_symbol*)malloc(I708_MAX_COLUMNS * sizeof(e708_symbol));
        if (!window.rows[i])
        {
          window.is_defined = 0;
          decoder->current_window = -1;
          for (int j = 0; j < i; j++)
          {
            free(window.rows[j]);
          }
          CLog::Log(LOGERROR, "{} - Not enough memory", __FUNCTION__);
          return;
        }
      }
      window.memory_reserved = 1;
    }
    window.is_defined = 1;
    WindowClearText(window);

    // Accorgind to CEA-708-D if window_style is 0 for newly created window , we have to apply predefined style #1
    if (window.win_style == 0)
      WindowApplyStyle(window, predefinitedWindowStyle[0]);
  }
  else
  {
    // Specs unclear here: Do we need to delete the text in the existing window?
    // We do this because one of the sample files demands it.
    WindowClearText(window);
  }
  // ...also makes the defined windows the current window (setCurrentWindow)
  handle_708_CWx_SetCurrentWindow(decoder, windowId);
  memcpy(window.commands, data + 1, 6);
}

void handle_708_SWA_SetWindowAttributes(cc708_service_decoder* decoder, unsigned char* data)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }
  int fill_color = (data[1]) & 0x3f;
  int fill_opacity = (data[1] >> 6) & 0x03;
  int border_color = (data[2]) & 0x3f;
  int border_type01 = (data[2] >> 6) & 0x03;
  int justify = (data[3]) & 0x03;
  int scroll_dir = (data[3] >> 2) & 0x03;
  int print_dir = (data[3] >> 4) & 0x03;
  int word_wrap = (data[3] >> 6) & 0x01;
  int border_type = ((data[3] >> 5) & 0x04) | border_type01;
  int display_eff = (data[4]) & 0x03;
  int effect_dir = (data[4] >> 2) & 0x03;
  int effect_speed = (data[4] >> 4) & 0x0f;

  e708Window& window = decoder->windows[decoder->current_window];
  window.attribs.fill_color = fill_color;
  window.attribs.fill_opacity = static_cast<WindowFillOpacity>(fill_opacity);
  window.attribs.border_color = border_color;
  window.attribs.justify = static_cast<WindowJustify>(justify);
  window.attribs.scroll_direction = static_cast<WindowScrollDirection>(scroll_dir);
  window.attribs.print_direction = static_cast<WindowPrintDirection>(print_dir);
  window.attribs.word_wrap = word_wrap;
  window.attribs.border_type = static_cast<WindowBorderType>(border_type);
  window.attribs.display_effect = static_cast<WindowScrollDisplayEffect>(display_eff);
  window.attribs.effect_direction = static_cast<WindowEffectDirection>(effect_dir);
  window.attribs.effect_speed = effect_speed;
}

void handle_708_DLW_DeleteWindows(cc708_service_decoder* decoder, int windowId)
{
  int screenContentChanged = 0;
  if (windowId == 0)
    CLog::Log(LOGDEBUG, "{} - No window ID", __FUNCTION__);
  else
  {
    for (int i = 0; i < I708_MAX_WINDOWS; i++)
    {
      if (windowId & 1)
      {
        e708Window& window = decoder->windows[i];

        if (window.is_defined && window.visible && !window.is_empty)
        {
          screenContentChanged = 1;
        }

        if (window.is_defined)
        {
          WindowClearText(window);
        }

        window.is_defined = 0;
        window.visible = 0;

        if (i == decoder->current_window)
        {
          // If the current window is deleted, then the decoder's current window ID
          // is unknown and must be reinitialized with either the SetCurrentWindow
          // or DefineWindow command.
          decoder->current_window = -1;
        }
      }
      windowId >>= 1;
    }
  }
  if (screenContentChanged && !DecoderHasVisibleWindows(decoder))
    updateScreen(decoder);
}

void handle_708_SPA_SetPenAttributes(cc708_service_decoder* decoder, unsigned char* data)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }

  int pen_size = (data[1]) & 0x3;
  int offset = (data[1] >> 2) & 0x3;
  int text_tag = (data[1] >> 4) & 0xf;
  int font_tag = (data[2]) & 0x7;
  int edge_type = (data[2] >> 3) & 0x7;
  int underline = (data[2] >> 4) & 0x1;
  int italic = (data[2] >> 5) & 0x1;

  e708Window& window = decoder->windows[decoder->current_window];
  if (window.pen_row == -1)
  {
    CLog::Log(LOGERROR, "{} - Can't set pen attribs for undefined row", __FUNCTION__);
    return;
  }

  e708Pen_attribs& pen = window.pen_attribs[window.pen_row];
  pen.pen_size = static_cast<PenSize>(pen_size);
  pen.offset = offset;
  pen.text_tag = static_cast<PenTextTag>(text_tag);
  pen.font_tag = static_cast<PenFontStyle>(font_tag);
  pen.edge_type = static_cast<PenEdgeType>(edge_type);
  pen.underline = underline;
  pen.italic = italic;
}

void handle_708_SPC_SetPenColor(cc708_service_decoder* decoder, unsigned char* data)
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }

  int fg_color = (data[1]) & 0x3f;
  int fg_opacity = (data[1] >> 6) & 0x03;
  int bg_color = (data[2]) & 0x3f;
  int bg_opacity = (data[2] >> 6) & 0x03;
  int edge_color = (data[3] >> 6) & 0x3f;

  e708Window& window = decoder->windows[decoder->current_window];
  if (window.pen_row == -1)
  {
    CLog::Log(LOGERROR, "{} - Can't set pen color for undefined row", __FUNCTION__);
    return;
  }

  e708Pen_color& pen = window.pen_colors[window.pen_row];
  pen.fg_color = fg_color;
  pen.fg_opacity = fg_opacity;
  pen.bg_color = bg_color;
  pen.bg_opacity = bg_opacity;
  pen.edge_color = edge_color;
}

void handle_708_SPL_SetPenLocation(cc708_service_decoder* decoder, unsigned char* data)
{
  int row = data[1] & 0x0f;
  int col = data[2] & 0x3f;
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return;
  }
  e708Window& window = decoder->windows[decoder->current_window];
  window.pen_row = row;
  window.pen_column = col;
}

//------------------------- SYNCHRONIZATION COMMANDS -------------------------

void handle_708_DLY_Delay(cc708_service_decoder* decoder, int tenths_of_sec)
{
  CLog::Log(LOGDEBUG,
            "{} - Delay command not implemented (requested delay for {} tenths of second)",
            __FUNCTION__, tenths_of_sec);
  //! @todo Probably ask for the current FTS and wait for this time before resuming -
  // not sure it's worth it though
}

void handle_708_DLC_DelayCancel(cc708_service_decoder* decoder)
{
  CLog::Log(LOGDEBUG, "{} - Delay cancel command not implemented", __FUNCTION__);
  //! @todo See above
}

//-------------------------- CHARACTERS AND COMMANDS -------------------------

int handle_708_C0_P16(cc708_service_decoder* decoder,
                         unsigned char* data) //16-byte chars always have 2 bytes
{
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return 3;
  }

  e708_symbol sym;

  if (data[0])
  {
    CCX_DTVCC_SYM_SET_16(sym, data[0], data[1]);
  }
  else
  {
    CCX_DTVCC_SYM_SET(sym, data[1]);
  }

  CLog::Log(LOGDEBUG, "{} - [CEA-708] _dtvcc_handle_C0_P16: {:04X}", __FUNCTION__, sym.sym);
  process_character(decoder, sym);

  return 3;
}

// G0 - Code Set - ASCII printable characters
int handle_708_G0(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  //! @todo Substitution of the music note character for the ASCII DEL character
  if (decoder->current_window == -1)
  {
    CLog::Log(LOGERROR, "{} - Window has to be defined first", __FUNCTION__);
    return data_length;
  }

  unsigned char c = get_internal_from_G0(data[0]);
  e708_symbol sym;
  CCX_DTVCC_SYM_SET(sym, c);
  process_character(decoder, sym);
  return 1;
}

// G1 Code Set - ISO 8859-1 LATIN-1 Character Set
int handle_708_G1(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  unsigned char c = get_internal_from_G1(data[0]);
  e708_symbol sym;
  CCX_DTVCC_SYM_SET(sym, c);
  process_character(decoder, sym);
  return 1;
}

int handle_708_C0(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  unsigned char c0 = data[0];
  const char* name = COMMANDS_C0[c0];
  if (!name)
    name = "Reserved";
  int len = -1;
  // These commands have a known length even if they are reserved.
  if (c0 <= 0xF)
  {
    switch (static_cast<CommandCodeC0>(c0))
    {
      case CommandCodeC0::CR:
        process_cr(decoder);
        break;
      case CommandCodeC0::HCR: // Horizontal Carriage Return
        process_hcr(decoder);
        break;
      case CommandCodeC0::FF: // Form Feed
        process_ff(decoder);
        break;
      case CommandCodeC0::ETX:
        process_etx(decoder);
        break;
    }
    len = 1;
  }
  else if (c0 >= 0x10 && c0 <= 0x17)
  {
    // Note that 0x10 is actually EXT1 and is dealt with somewhere else. Rest is undefined as per
    // CEA-708-D
    len = 2;
  }
  else if (c0 >= 0x18 && c0 <= 0x1F)
  {
    // Only PE16 is defined.
    if (static_cast<CommandCodeC0>(c0) == CommandCodeC0::P16)
    {
      handle_708_C0_P16(decoder, data + 1);
    }
    len = 3;
  }
  if (len == -1)
  {
    CLog::Log(LOGERROR, "{} - Wrong len == -1", __FUNCTION__);
    return -1;
  }
  if (len > data_length)
  {
    CLog::Log(LOGERROR, "{} - Command is {} bytes long but we only have {}", __FUNCTION__, len,
              data_length);
    return -1;
  }
  return len;
}

// C1 Code Set - Captioning Commands Control Codes
int handle_708_C1(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  struct commandC1 com = commandsC1[data[0] - 0x80];
  if (com.length > data_length)
  {
    return -1;
  }
  switch (com.code)
  {
    case CommandCodeC1::CW0: /* SetCurrentWindow */
    case CommandCodeC1::CW1:
    case CommandCodeC1::CW2:
    case CommandCodeC1::CW3:
    case CommandCodeC1::CW4:
    case CommandCodeC1::CW5:
    case CommandCodeC1::CW6:
    case CommandCodeC1::CW7:
    {
      int windowId = static_cast<int>(com.code) - static_cast<int>(CommandCodeC1::CW0);
      handle_708_CWx_SetCurrentWindow(decoder, windowId); /* Window 0 to 7 */
      break;
    }
    case CommandCodeC1::CLW:
      handle_708_CLW_ClearWindows(decoder, data[1]);
      break;
    case CommandCodeC1::DSW:
      handle_708_DSW_DisplayWindows(decoder, data[1]);
      break;
    case CommandCodeC1::HDW:
      handle_708_HDW_HideWindows(decoder, data[1]);
      break;
    case CommandCodeC1::TGW:
      handle_708_TGW_ToggleWindows(decoder, data[1]);
      break;
    case CommandCodeC1::DLW:
      handle_708_DLW_DeleteWindows(decoder, data[1]);
      break;
    case CommandCodeC1::DLY:
      handle_708_DLY_Delay(decoder, data[1]);
      break;
    case CommandCodeC1::DLC:
      handle_708_DLC_DelayCancel(decoder);
      break;
    case CommandCodeC1::RST:
      ServiceReset(decoder);
      break;
    case CommandCodeC1::SPA:
      handle_708_SPA_SetPenAttributes(decoder, data);
      break;
    case CommandCodeC1::SPC:
      handle_708_SPC_SetPenColor(decoder, data);
      break;
    case CommandCodeC1::SPL:
      handle_708_SPL_SetPenLocation(decoder, data);
      break;
    case CommandCodeC1::RSV93:
    case CommandCodeC1::RSV94:
    case CommandCodeC1::RSV95:
    case CommandCodeC1::RSV96:
      break;
    case CommandCodeC1::SWA:
      handle_708_SWA_SetWindowAttributes(decoder, data);
      break;
    case CommandCodeC1::DF0:
    case CommandCodeC1::DF1:
    case CommandCodeC1::DF2:
    case CommandCodeC1::DF3:
    case CommandCodeC1::DF4:
    case CommandCodeC1::DF5:
    case CommandCodeC1::DF6:
    case CommandCodeC1::DF7:
    {
      int windowId = static_cast<int>(com.code) - static_cast<int>(CommandCodeC1::DF0);
      handle_708_DFx_DefineWindow(decoder, windowId, data); /* Window 0 to 7 */
      break;
    }
    default:
      break;
  }

  return com.length;
}

/* This function handles future codes. While by definition we can't do any work on them, we must return
how many bytes would be consumed if these codes were supported, as defined in the specs.
Note: EXT1 not included */
// C2: Extended Miscellaneous Control Codes
//! @todo This code is completely untested due to lack of samples. Just following specs!
int handle_708_C2(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  if (data[0] <= 0x07) // 00-07...
    return 1; // ... Single-byte control bytes (0 additional bytes)
  else if (data[0] <= 0x0f) // 08-0F ...
    return 2; // ..two-byte control codes (1 additional byte)
  else if (data[0] <= 0x17) // 10-17 ...
    return 3; // ..three-byte control codes (2 additional bytes)
  return 4; // 18-1F => four-byte control codes (3 additional bytes)
}

int handle_708_C3(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  if (data[0] < 0x80 || data[0] > 0x9F)
    CLog::Log(LOGERROR, "{} - Entry in handle_708_C3 with an out of range value", __FUNCTION__);
  if (data[0] <= 0x87) // 80-87...
    return 5; // ... Five-byte control bytes (4 additional bytes)
  else if (data[0] <= 0x8F) // 88-8F ...
    return 6; // ..Six-byte control codes (5 additional byte)
  // If here, then 90-9F ...

  // These are variable length commands, that can even span several segments
  // (they allow even downloading fonts or graphics).
  //! @todo Implement if a sample ever appears
  CLog::Log(LOGWARNING, "{} - [CEA-708] This sample contains unsupported 708 data", __FUNCTION__);
  return 0; // Unreachable, but otherwise there's compilers warnings
}

// This function handles extended codes (EXT1 + code), from the extended sets
// G2 (20-7F) => Mostly unmapped, except for a few characters.
// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708-B
// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
//! @todo This code is completely untested due to lack of samples. Just following specs!
// Returns number of used bytes, usually 1 (since EXT1 is not counted).
int handle_708_extended_char(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  int used;
  unsigned char c = 0x20; // Default to space
  unsigned char code = data[0];
  if (/* data[i]>=0x00 && */ code <= 0x1F) // Comment to silence warning
  {
    used = handle_708_C2(decoder, data, data_length);
  }
  // Group G2 - Extended Miscellaneous Characters
  else if (code >= 0x20 && code <= 0x7F)
  {
    c = get_internal_from_G2(code);
    used = 1;
    e708_symbol sym;
    CCX_DTVCC_SYM_SET(sym, c);
    process_character(decoder, sym);
  }
  // Group C3
  else if (code >= 0x80 && code <= 0x9F)
  {
    used = handle_708_C3(decoder, data, data_length);
    //! @todo Something
  }
  // Group G3
  else
  {
    c = get_internal_from_G3(code);
    used = 1;
    e708_symbol sym;
    CCX_DTVCC_SYM_SET(sym, c);
    process_character(decoder, sym);
  }
  return used;
}

//------------------------------- PROCESSING --------------------------------

void process_service_block(cc708_service_decoder* decoder, unsigned char* data, int data_length)
{
  int i = 0;
  while (i < data_length)
  {
    int used = -1;
    if (data[i] != static_cast<char>(CommandCodeC0::EXT1))
    {
      // Group C0
      if (/* data[i]>=0x00 && */ data[i] <= 0x1F) // Comment to silence warning
      {
        used = handle_708_C0(decoder, data + i, data_length - i);
      }
      // Group G0
      else if (data[i] >= 0x20 && data[i] <= 0x7F)
      {
        used = handle_708_G0(decoder, data + i, data_length - i);
      }
      // Group C1
      else if (data[i] >= 0x80 && data[i] <= 0x9F)
      {
        used = handle_708_C1(decoder, data + i, data_length - i);
      }
      // Group C2
      else
        used = handle_708_G1(decoder, data + i, data_length - i);
      if (used == -1)
      {
        CLog::Log(LOGERROR, "{} - There was a problem handling the data. Reseting service decoder",
                  __FUNCTION__);
        //! @todo Not sure if a local reset is going to be helpful here.
        ServiceReset(decoder);
        return;
      }
    }
    else // Use extended set
    {
      used = handle_708_extended_char(decoder, data + i + 1, data_length - 1);
      used++; // Since we had EXT1
    }
    i += used;
  }

  // update rollup windows
  int update = 0;
  for (e708Window& window : decoder->windows)
  {
    if (window.is_defined && window.visible &&
        (window.anchor_point == PenAnchorPoint::BOTTOM_LEFT ||
         window.anchor_point == PenAnchorPoint::BOTTOM_CENTER))
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

void process_current_packet(cc708_service_decoder* decoders, int len)
{
  int seq = (decoders[0].parent->m_current_packet[0] & 0xC0) >> 6; // Two most significants bits

  if (decoders[0].parent->m_current_packet_length == 0)
    return;

  if (decoders[0].parent->m_current_packet_length != len) // Is this possible?
  {
    DecodersReset(decoders);
    return;
  }
  int last_seq = decoders[0].parent->m_last_sequence;
  if ((last_seq != CC708_NO_LAST_SEQUENCE) && ((last_seq + 1) % 4 != seq))
  {
    DecodersReset(decoders);
    return;
  }
  decoders[0].parent->m_last_sequence = seq;

  unsigned char* pos = decoders[0].parent->m_current_packet + 1;

  while (pos < decoders[0].parent->m_current_packet + len)
  {
    int service_number = (pos[0] & 0xE0) >> 5; // 3 more significant bits
    int block_length = (pos[0] & 0x1F); // 5 less significant bits

    if (service_number == 7) // There is an extended header
    {
      pos++;
      service_number = (pos[0] & 0x3F); // 6 more significant bits
      if (service_number < 7)
      {
        CLog::Log(LOGERROR, "{} - Illegal service number in extended header: {}", __FUNCTION__,
                  service_number);
      }
      pos = decoders[0].parent->m_current_packet + len;
      break;
    }

    pos++; // Move to service data
    if (service_number == 0 && block_length != 0) // Illegal, but specs say what to do...
    {
      CLog::Log(LOGDEBUG, "{} - Data received for service 0, skipping rest of packet",
                __FUNCTION__);
      pos = decoders[0].parent->m_current_packet + len; // Move to end
      break;
    }

    if (service_number > 0 && decoders->parent->m_servicesActive[service_number])
      process_service_block(&decoders[service_number], pos, block_length);

    pos += block_length; // Skip data
  }

  clear_packet(&decoders[0]);

  if (pos != decoders[0].parent->m_current_packet +
                 len) // For some reason we didn't parse the whole packet
  {
    CLog::Log(LOGDEBUG, "{} - There was a problem with this packet, resetting", __FUNCTION__);
    DecodersReset(decoders);
  }

  if (len < 128 && *pos) // Null header is mandatory if there is room
  {
    CLog::Log(LOGWARNING, "{} - Null header expected but not found", __FUNCTION__);
  }
}

void decode_708(unsigned char* data, int dataLength, cc708_service_decoder* decoders)
{
  /*
   * Note: the data has following format:
   * 1 byte for cc_valid
   * 1 byte for cc_type
   * 2 bytes for the actual data
    */
  for (int i = 0; i < dataLength; i += 3)
  {
    unsigned char cc_valid = (data[i] & 0x04) >> 2;
    unsigned char cc_type = data[i] & 0x03;
    bool skipHeader = false;
    if (cc_valid == 1)
    {
      // Send the packet to the appropriate data array (CEA-608 or CEA-708)
      switch (cc_type)
      {
        case NTSC_CC_FIELD_1: // CEA-608 NTSC data
          if (!decoders[0].parent->m_seen708)
          {
            decode_cc(decoders[0].parent->m_cc608decoder, (const uint8_t*)data + i, 3);
          }
          break;
        case NTSC_CC_FIELD_2:
          // FIXME: This case should be handled as NTSC_CC_FIELD_1 case,
          // but not works ("karaoke" style is broken)
          skipHeader = true;
        case DTVCC_PACKET_DATA: // CEA-708 DTVCC packet data
          if (decoders[0].parent->m_is_current_packet_header_parsed || skipHeader)
          {
            if (decoders[0].parent->m_current_packet_length > 253)
            {
              CLog::Log(LOGWARNING, "{} - Legal packet size exceeded (case 2), data not added",
                        __FUNCTION__);
            }
            else
            {
              //! @todo m_current_packet_length can exceed m_current_packet size
              decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++] =
                  data[i + 1];
              decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++] =
                  data[i + 2];

              int len = decoders[0].parent->m_current_packet[0] & 0x3F; // 6 least significants bits

              if (len == 0) // This is well defined in EIA-708; no magic.
                len = 128;
              else
                len = len * 2;
              // Note that len here is the length including the header
              if (decoders[0].parent->m_current_packet_length >= len)
                process_current_packet(decoders, len);
            }
          }
          break;
        case DTVCC_PACKET_START: // CEA-708 DTVCC header data
          if (decoders[0].parent->m_current_packet_length > CC708_MAX_PACKET_LENGTH - 1)
          {
            CLog::Log(LOGWARNING, "{} - Legal packet size exceeded (case 3), data not added",
                      __FUNCTION__);
          }
          else
          {
            decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++] =
                data[i + 1];
            decoders[0].parent->m_current_packet[decoders[0].parent->m_current_packet_length++] =
                data[i + 2];
            decoders[0].parent->m_is_current_packet_header_parsed = true;
          }
          break;
        default:
          // Should not happen
          CLog::Log(LOGDEBUG, "{} - Unhandled cc_type: {}", __FUNCTION__, cc_type);
          break;
      }
    }
  }
}

void ccx_decoders_708_init(cc708_service_decoder *decoders, void (*handler)(int service, void *userdata), void *userdata, CDecoderCC708 *parent)
{
  for (int i = 0; i < CC708_MAX_SERVICES; i++)
  {
    ServiceReset(&decoders[i]);
    decoders[i].cc_count = 0;
    decoders[i].tv.service_number = i + 1;
    decoders[i].tv.cc_count = 0;
    decoders[i].callback = handler;
    decoders[i].userdata = userdata;
    decoders[i].parent = parent;
  }
  // Mark all services active
  memset(decoders[0].parent->m_servicesActive, 1, sizeof(decoders[0].parent->m_servicesActive));

  decoders[0].parent->m_cc608decoder->callback = handler;
  decoders[0].parent->m_cc608decoder->userdata = userdata;

  decoders[0].parent->m_current_packet_length = 0;
  decoders[0].parent->m_is_current_packet_header_parsed = false;
  decoders[0].parent->m_last_sequence = CC708_NO_LAST_SEQUENCE;
  decoders[0].parent->m_seen708 = false;
  decoders[0].parent->m_seen608 = false;
  decoders[0].parent->m_no_rollup = false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CDecoderCC708::CDecoderCC708()
{
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
  m_cc708decoders = new cc708_service_decoder[CC708_MAX_SERVICES];
  ccx_decoders_708_init(m_cc708decoders, handler, userdata, this);
}

void CDecoderCC708::Decode(unsigned char *data, int datalength)
{
  decode_708(data, datalength, m_cc708decoders);
}
