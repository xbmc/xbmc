/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/Key.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIListItem.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/DirtyRegion.h"
#include <tinyxml.h>
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "threads/SystemClock.h"
#include "GUIInfoManager.h"

#include "epg/Epg.h"
#include "pvr/channels/PVRChannel.h"

#include "GUIEPGGridContainer.h"

using namespace PVR;
using namespace EPG;
using namespace std;

#define SHORTGAP     5 // how many blocks is considered a short-gap in nav logic
#define MINSPERBLOCK 5 /// would be nice to offer zooming of busy schedules /// performance cost to increase resolution 5 fold?
#define BLOCKJUMP    4 // how many blocks are jumped with each analogue scroll action
#define BLOCK_SCROLL_OFFSET 60 / MINSPERBLOCK // how many blocks are jumped if we are at left/right edge of grid

CGUIEPGGridContainer::CGUIEPGGridContainer(int parentID, int controlID, float posX, float posY, float width,
                                           float height, int scrollTime, int preloadItems, int timeBlocks, int rulerUnit,
                                           const CTextureInfo& progressIndicatorTexture)
    : IGUIContainer(parentID, controlID, posX, posY, width, height)
    , m_guiProgressIndicatorTexture(posX, posY, width, height, progressIndicatorTexture)
{
  ControlType             = GUICONTAINER_EPGGRID;
  m_blocksPerPage         = timeBlocks;
  m_rulerUnit             = rulerUnit;
  m_channelCursor         = 0;
  m_blockCursor           = 0;
  m_channelOffset         = 0;
  m_blockOffset           = 0;
  m_channelScrollOffset   = 0;
  m_channelScrollSpeed    = 0;
  m_channelScrollLastTime = 0;
  m_programmeScrollOffset = 0;
  m_programmeScrollSpeed  = 0;
  m_programmeScrollLastTime  = 0;
  m_programmesPerPage     = 0;
  m_channelsPerPage       = 0;
  m_channels              = 0;
  m_blocks                = 0;
  m_scrollTime            = scrollTime ? scrollTime : 1;
  m_item                  = NULL;
  m_lastItem              = NULL;
  m_lastChannel           = NULL;
  m_programmeLayout       = NULL;
  m_focusedProgrammeLayout= NULL;
  m_channelLayout         = NULL;
  m_focusedChannelLayout  = NULL;
  m_rulerLayout           = NULL;
  m_rulerPosX             = 0;
  m_rulerPosY             = 0;
  m_rulerHeight           = 0;
  m_rulerWidth            = 0;
  m_channelPosX           = 0;
  m_channelPosY           = 0;
  m_channelHeight         = 0;
  m_channelWidth          = 0;
  m_gridPosX              = 0;
  m_gridPosY              = 0;
  m_gridWidth             = 0;
  m_gridHeight            = 0;
  m_blockSize             = 0;
  m_analogScrollCount     = 0;
  m_cacheChannelItems     = preloadItems;
  m_cacheRulerItems       = preloadItems;
  m_cacheProgrammeItems   = preloadItems;
  m_wasReset              = false;
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
  Reset();
}

void CGUIEPGGridContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  UpdateScrollOffset(currentTime);
  ProcessChannels(currentTime, dirtyregions);
  ProcessRuler(currentTime, dirtyregions);
  ProcessProgrammeGrid(currentTime, dirtyregions);
  ProcessProgressIndicator(currentTime, dirtyregions);

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIEPGGridContainer::Render()
{
  RenderChannels();
  RenderRuler();
  RenderProgrammeGrid();
  RenderProgressIndicator();

  CGUIControl::Render();
}

void CGUIEPGGridContainer::ProcessChannels(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_focusedChannelLayout || !m_channelLayout)
    return;

  int chanOffset  = (int)floorf(m_channelScrollOffset / m_programmeLayout->Size(VERTICAL));

  int cacheBeforeChannel, cacheAfterChannel;
  GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

  // Free memory not used on screen
  if ((int)m_channelItems.size() > m_channelsPerPage + cacheBeforeChannel + cacheAfterChannel)
    FreeChannelMemory(chanOffset - cacheBeforeChannel, chanOffset + m_channelsPerPage + 1 + cacheAfterChannel);

  CPoint originChannel = CPoint(m_channelPosX, m_channelPosY) + m_renderOffset;
  float pos = originChannel.y;
  float end = m_posY + m_height;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (chanOffset - cacheBeforeChannel) * m_channelLayout->Size(VERTICAL) - m_channelScrollOffset;
  if (m_channelOffset + m_channelCursor < chanOffset)
    drawOffset += m_focusedChannelLayout->Size(VERTICAL) - m_channelLayout->Size(VERTICAL);
  pos += drawOffset;
  end += cacheAfterChannel * m_channelLayout->Size(VERTICAL);

  int current = chanOffset;// - cacheBeforeChannel;
  while (pos < end && !m_channelItems.empty())
  {
    int itemNo = current;
    if (itemNo >= (int)m_channelItems.size())
      break;
    bool focused = (current == m_channelOffset + m_channelCursor);
    if (itemNo >= 0)
    {
      CGUIListItemPtr item = m_channelItems[itemNo];
      // process our item
      ProcessItem(originChannel.x, pos, item.get(), m_lastItem, focused, m_channelLayout, m_focusedChannelLayout, currentTime, dirtyregions);
    }
    // increment our position
    pos += focused ? m_focusedChannelLayout->Size(VERTICAL) : m_channelLayout->Size(VERTICAL);
    current++;
  }
}

void CGUIEPGGridContainer::RenderChannels()
{
  if (!m_focusedChannelLayout || !m_channelLayout)
    return;

  int chanOffset  = (int)floorf(m_channelScrollOffset / m_programmeLayout->Size(VERTICAL));

  /// Render channel names
  int cacheBeforeChannel, cacheAfterChannel;
  GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

  g_graphicsContext.SetClipRegion(m_channelPosX, m_channelPosY, m_channelWidth, m_gridHeight);

  CPoint originChannel = CPoint(m_channelPosX, m_channelPosY) + m_renderOffset;
  float pos = originChannel.y;
  float end = m_posY + m_height;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (chanOffset - cacheBeforeChannel) * m_channelLayout->Size(VERTICAL) - m_channelScrollOffset;
  if (m_channelOffset + m_channelCursor < chanOffset)
    drawOffset += m_focusedChannelLayout->Size(VERTICAL) - m_channelLayout->Size(VERTICAL);
  pos += drawOffset;
  end += cacheAfterChannel * m_channelLayout->Size(VERTICAL);

  float focusedPos = 0;
  CGUIListItemPtr focusedItem;
  int current = chanOffset;// - cacheBeforeChannel;
  while (pos < end && !m_channelItems.empty())
  {
    int itemNo = current;
    if (itemNo >= (int)m_channelItems.size())
      break;
    bool focused = (current == m_channelOffset + m_channelCursor);
    if (itemNo >= 0)
    {
      CGUIListItemPtr item = m_channelItems[itemNo];
      // render our item
      if (focused)
      {
        focusedPos = pos;
        focusedItem = item;
      }
      else
      {
        RenderItem(originChannel.x, pos, item.get(), false);
      }
    }
    // increment our position
    pos += focused ? m_focusedChannelLayout->Size(VERTICAL) : m_channelLayout->Size(VERTICAL);
    current++;
  }
  // render focused item last so it can overlap other items
  if (focusedItem)
  {
    RenderItem(originChannel.x, focusedPos, focusedItem.get(), true);
  }
  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::ProcessRuler(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_rulerLayout || m_rulerItems.size()<=1 || (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0))
    return;

  int rulerOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);
  CGUIListItemPtr item = m_rulerItems[0];
  item->SetLabel(m_rulerItems[rulerOffset/m_rulerUnit+1]->GetLabel2());
  CGUIListItem* lastitem = NULL; // dummy pointer needed to be passed as reference to ProcessItem() method
  ProcessItem(m_posX, m_posY, item.get(), lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_channelWidth);

  // render ruler items
  int cacheBeforeRuler, cacheAfterRuler;
  GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

  // Free memory not used on screen
  if ((int)m_rulerItems.size() > m_blocksPerPage + cacheBeforeRuler + cacheAfterRuler)
    FreeRulerMemory(rulerOffset/m_rulerUnit+1 - cacheBeforeRuler, rulerOffset/m_rulerUnit+1 + m_blocksPerPage + 1 + cacheAfterRuler);

  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  float pos = originRuler.x;
  float end = m_posX + m_width;
  float drawOffset = (rulerOffset - cacheBeforeRuler) * m_blockSize - m_programmeScrollOffset;
  pos += drawOffset;
  end += cacheAfterRuler * m_rulerLayout->Size(HORIZONTAL);

  if (rulerOffset % m_rulerUnit != 0)
  {
    /* first ruler marker starts before current view */
    int startBlock = rulerOffset - 1;

    while (startBlock % m_rulerUnit != 0)
      startBlock--;

    int missingSection = rulerOffset - startBlock;

    pos -= missingSection * m_blockSize;
  }
  while (pos < end && (rulerOffset/m_rulerUnit+1) < (int)m_rulerItems.size())
  {
    item = m_rulerItems[rulerOffset/m_rulerUnit+1];
    ProcessItem(pos, originRuler.y, item.get(), lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_rulerWidth);
    pos += m_rulerWidth;
    rulerOffset += m_rulerUnit;
  }
}

void CGUIEPGGridContainer::RenderRuler()
{
  if (!m_rulerLayout || m_rulerItems.size()<=1 || (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0))
    return;

  int rulerOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);

  /// Render single ruler item with date of selected programme
  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  CGUIListItemPtr item = m_rulerItems[0];
  RenderItem(m_posX, m_posY, item.get(), false);
  g_graphicsContext.RestoreClipRegion();

  // render ruler items
  int cacheBeforeRuler, cacheAfterRuler;
  GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

  g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_gridWidth, m_rulerHeight);

  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  float pos = originRuler.x;
  float end = m_posX + m_width;
  float drawOffset = (rulerOffset - cacheBeforeRuler) * m_blockSize - m_programmeScrollOffset;
  pos += drawOffset;
  end += cacheAfterRuler * m_rulerLayout->Size(HORIZONTAL);

  if (rulerOffset % m_rulerUnit != 0)
  {
    /* first ruler marker starts before current view */
    int startBlock = rulerOffset - 1;

    while (startBlock % m_rulerUnit != 0)
      startBlock--;

    int missingSection = rulerOffset - startBlock;

    pos -= missingSection * m_blockSize;
  }
  while (pos < end && (rulerOffset/m_rulerUnit+1) < (int)m_rulerItems.size())
  {
    item = m_rulerItems[rulerOffset/m_rulerUnit+1];
    RenderItem(pos, originRuler.y, item.get(), false);
    pos += m_rulerWidth;
    rulerOffset += m_rulerUnit;
  }
  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::ProcessProgrammeGrid(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_focusedProgrammeLayout || !m_programmeLayout || m_rulerItems.size()<=1 || (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0))
    return;

  int blockOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);
  int chanOffset  = (int)floorf(m_channelScrollOffset / m_programmeLayout->Size(VERTICAL));

  int cacheBeforeProgramme, cacheAfterProgramme;
  GetProgrammeCacheOffsets(cacheBeforeProgramme, cacheAfterProgramme);

  CPoint originProgramme = CPoint(m_gridPosX, m_gridPosY) + m_renderOffset;
  float posA = originProgramme.x;
  float endA = m_posX + m_width;
  float posB = originProgramme.y;
  float endB = m_gridPosY + m_gridHeight;
  endA += cacheAfterProgramme * m_blockSize;

  float DrawOffsetA = blockOffset * m_blockSize - m_programmeScrollOffset;
  posA += DrawOffsetA;
  float DrawOffsetB = (chanOffset - cacheBeforeProgramme) * m_channelLayout->Size(VERTICAL) - m_channelScrollOffset;
  posB += DrawOffsetB;

  int channel = chanOffset;

  while (posB < endB && !m_channelItems.empty())
  {
    if (channel >= (int)m_channelItems.size())
      break;

    // Free memory not used on screen
    FreeProgrammeMemory(channel, blockOffset - cacheBeforeProgramme, blockOffset + m_programmesPerPage + 1 + cacheAfterProgramme);

    int block = blockOffset;
    float posA2 = posA;

    CGUIListItemPtr item = m_gridIndex[channel][block].item;
    if (blockOffset > 0 && item == m_gridIndex[channel][blockOffset-1].item)
    {
      /* first program starts before current view */
      int startBlock = blockOffset - 1;
      while (startBlock >= 0 && m_gridIndex[channel][startBlock].item == item)
        startBlock--;

      block = startBlock + 1;
      int missingSection = blockOffset - block;
      posA2 -= missingSection * m_blockSize;
    }

    while (posA2 < endA && !m_programmeItems.empty())   // FOR EACH ITEM ///////////////
    {
      item = m_gridIndex[channel][block].item;
      if (!item || !item.get()->IsFileItem())
        break;

      bool focused = (channel == m_channelOffset + m_channelCursor) && (item == m_gridIndex[m_channelOffset + m_channelCursor][m_blockOffset + m_blockCursor].item);

      // calculate the size to truncate if item is out of grid view
      float truncateSize = 0;
      if (posA2 < posA)
      {
        truncateSize = posA - posA2;
        posA2 = posA; // reset to grid start position
      }

      // truncate item's width
      m_gridIndex[channel][block].width = m_gridIndex[channel][block].originWidth - truncateSize;

      ProcessItem(posA2, posB, item.get(), m_lastChannel, focused, m_programmeLayout, m_focusedProgrammeLayout, currentTime, dirtyregions, m_gridIndex[channel][block].width);

      // increment our X position
      posA2 += m_gridIndex[channel][block].width; // assumes focused & unfocused layouts have equal length
      block += (int)(m_gridIndex[channel][block].originWidth / m_blockSize);
    }

    // increment our Y position
    channel++;
    posB += m_channelHeight;
  }
}

void CGUIEPGGridContainer::RenderProgrammeGrid()
{
  if (!m_focusedProgrammeLayout || !m_programmeLayout || m_rulerItems.size()<=1 || (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0))
    return;

  int blockOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);
  int chanOffset  = (int)floorf(m_channelScrollOffset / m_programmeLayout->Size(VERTICAL));

  /// Render programmes
  int cacheBeforeProgramme, cacheAfterProgramme;
  GetProgrammeCacheOffsets(cacheBeforeProgramme, cacheAfterProgramme);

  g_graphicsContext.SetClipRegion(m_gridPosX, m_gridPosY, m_gridWidth, m_gridHeight);
  CPoint originProgramme = CPoint(m_gridPosX, m_gridPosY) + m_renderOffset;
  float posA = originProgramme.x;
  float endA = m_posX + m_width;
  float posB = originProgramme.y;
  float endB = m_gridPosY + m_gridHeight;
  endA += cacheAfterProgramme * m_blockSize;

  float DrawOffsetA = blockOffset * m_blockSize - m_programmeScrollOffset;
  posA += DrawOffsetA;
  float DrawOffsetB = (chanOffset - cacheBeforeProgramme) * m_channelLayout->Size(VERTICAL) - m_channelScrollOffset;
  posB += DrawOffsetB;

  int channel = chanOffset;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItemPtr focusedItem;
  while (posB < endB && !m_channelItems.empty())
  {
    if (channel >= (int)m_channelItems.size())
      break;

    int block = blockOffset;
    float posA2 = posA;

    CGUIListItemPtr item = m_gridIndex[channel][block].item;
    if (blockOffset > 0 && item == m_gridIndex[channel][blockOffset-1].item)
    {
      /* first program starts before current view */
      int startBlock = blockOffset - 1;
      while (startBlock >= 0 && m_gridIndex[channel][startBlock].item == item)
        startBlock--;

      block = startBlock + 1;
      int missingSection = blockOffset - block;
      posA2 -= missingSection * m_blockSize;
    }

    while (posA2 < endA && !m_programmeItems.empty())   // FOR EACH ITEM ///////////////
    {
      item = m_gridIndex[channel][block].item;
      if (!item || !item.get()->IsFileItem())
        break;

      bool focused = (channel == m_channelOffset + m_channelCursor) && (item == m_gridIndex[m_channelOffset + m_channelCursor][m_blockOffset + m_blockCursor].item);

      // reset to grid start position if first item is out of grid view
      if (posA2 < posA)
        posA2 = posA;

      // render our item
      if (focused)
      {
        focusedPosX = posA2;
        focusedPosY = posB;
        focusedItem = item;
      }
      else
      {
        RenderItem(posA2, posB, item.get(), focused);
      }

      // increment our X position
      posA2 += m_gridIndex[channel][block].width; // assumes focused & unfocused layouts have equal length
      block += (int)(m_gridIndex[channel][block].originWidth / m_blockSize);
    }

    // increment our Y position
    channel++;
    posB += m_channelHeight;
  }

  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem.get(), true);

  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::ProcessProgressIndicator(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  float width = ((CDateTime::GetUTCDateTime() - m_gridStart).GetSecondsTotal() * m_blockSize) / (MINSPERBLOCK * 60) - m_programmeScrollOffset;

  if (width > 0)
  {
    m_guiProgressIndicatorTexture.SetVisible(true);
    m_guiProgressIndicatorTexture.SetPosition(originRuler.x, originRuler.y);
    m_guiProgressIndicatorTexture.SetWidth(width);
  }
  else
  {
    m_guiProgressIndicatorTexture.SetVisible(false);
  }
  
  m_guiProgressIndicatorTexture.Process(currentTime);
}

void CGUIEPGGridContainer::RenderProgressIndicator()
{
  if (g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_gridWidth, m_height))
  {
    m_guiProgressIndicatorTexture.Render();
    g_graphicsContext.RestoreClipRegion();
  }
}

void CGUIEPGGridContainer::ProcessItem(float posX, float posY, CGUIListItem* item, CGUIListItem *&lastitem,
  bool focused, CGUIListItemLayout* normallayout, CGUIListItemLayout* focusedlayout,
  unsigned int currentTime, CDirtyRegionList &dirtyregions, float resize /* = -1.0f */)
{
  if (!normallayout || !focusedlayout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*focusedlayout);
      item->SetFocusedLayout(layout);
    }

    if (resize != -1.0f)
    {
      item->GetFocusedLayout()->SetWidth(resize);
    }

    if (item != lastitem || !HasFocus())
      item->GetFocusedLayout()->SetFocusedItem(0);

    if (item != lastitem && HasFocus())
    {
      item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);
      unsigned int subItem = 1;
      if (lastitem && lastitem->GetFocusedLayout())
        subItem = lastitem->GetFocusedLayout()->GetFocusedItem();
      item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
    }

    item->GetFocusedLayout()->Process(item, m_parentID, currentTime, dirtyregions);
    lastitem = item;
  }
  else
  {
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*normallayout);
      item->SetLayout(layout);
    }

    if (resize != -1.0f)
    {
      item->GetLayout()->SetWidth(resize);
    }

    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);

    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Process(item, m_parentID, currentTime, dirtyregions);
    else
      item->GetLayout()->Process(item, m_parentID, currentTime, dirtyregions);
  }
  g_graphicsContext.RestoreOrigin();
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (focused)
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->Render(item, m_parentID);
  }
  else
  {
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item, m_parentID);
    else if (item->GetLayout())
      item->GetLayout()->Render(item, m_parentID);
  }
  g_graphicsContext.RestoreOrigin();
}

bool CGUIEPGGridContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_DOWN:
    case ACTION_MOVE_UP:
    case ACTION_NAV_BACK:
      // use base class implementation
      return CGUIControl::OnAction(action);

    case ACTION_NEXT_ITEM:
      // skip +12h
      ScrollToBlockOffset(m_blockOffset + (12 * 60  / MINSPERBLOCK));
      return true;

    case ACTION_PREV_ITEM:
      // skip -12h
      ScrollToBlockOffset(m_blockOffset - (12 * 60 / MINSPERBLOCK));
      return true;

    case ACTION_PAGE_UP:
      if (m_channelOffset == 0)
      { // already on the first page, so move to the first item
        SetChannel(0);
      }
      else
      { // scroll up to the previous page
        ChannelScroll(m_channelsPerPage*-1);
      }
      return true;

    case ACTION_PAGE_DOWN:
      if (m_channelOffset == m_channels - m_channelsPerPage || m_channels < m_channelsPerPage)
      { // already at the last page, so move to the last item.
        SetChannel(m_channels - m_channelOffset - 1);
      }
      else
      { // scroll down to the next page
        ChannelScroll(m_channelsPerPage);
      }
      return true;

    // smooth scrolling (for analog controls)
    case ACTION_TELETEXT_RED:
    case ACTION_TELETEXT_GREEN:
    case ACTION_SCROLL_UP: // left horizontal scrolling
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_RED ? m_blocksPerPage/2 : m_blocksPerPage/4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset > 0 && m_blockCursor <= m_blocksPerPage / 2)
          {
            ProgrammesScroll(-blocksToJump);
          }
          else if (m_blockCursor > blocksToJump)
          {
            SetBlock(m_blockCursor - blocksToJump);
          }
        }

        return handled;
      }
      break;

    case ACTION_TELETEXT_BLUE:
    case ACTION_TELETEXT_YELLOW:
    case ACTION_SCROLL_DOWN: // right horizontal scrolling
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_BLUE ? m_blocksPerPage/2 : m_blocksPerPage/4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset + m_blocksPerPage < m_blocks && m_blockCursor >= m_blocksPerPage / 2)
          {
            ProgrammesScroll(blocksToJump);
          }
          else if (m_blockCursor < m_blocksPerPage - blocksToJump && m_blockOffset + m_blockCursor < m_blocks - blocksToJump)
          {
            SetBlock(m_blockCursor + blocksToJump);
          }
        }

        return handled;
      }
      break;

    default:
      if (action.GetID())
        return OnClick(action.GetID());
      break;
  }

  return false;
}

bool CGUIEPGGridContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    switch (message.GetMessage())
    {
      case GUI_MSG_ITEM_SELECTED:
        message.SetParam1(GetSelectedItem());
        return true;

      case GUI_MSG_LABEL_BIND:
        if (message.GetPointer())
        {
          Reset();
          CFileItemList *items = (CFileItemList *)message.GetPointer();

          /* Create programme items */
          m_programmeItems.reserve(items->Size());
          for (int i = 0; i < items->Size(); i++)
          {
            CFileItemPtr fileItem = items->Get(i);
            if (fileItem->HasEPGInfoTag() && fileItem->GetEPGInfoTag()->HasPVRChannel())
              m_programmeItems.push_back(fileItem);
          }

          /* Create Channel items */
          int iLastChannelID = -1;
          ItemsPtr itemsPointer;
          itemsPointer.start = 0;
          for (unsigned int i = 0; i < m_programmeItems.size(); ++i)
          {
            const CEpgInfoTag* tag = ((CFileItem*)m_programmeItems[i].get())->GetEPGInfoTag();
            CPVRChannelPtr channel = tag->ChannelTag();
            if (!channel)
              continue;
            int iCurrentChannelID = channel->ChannelID();
            if (iCurrentChannelID != iLastChannelID)
            {
              if (i > 0)
              {
                itemsPointer.stop = i-1;
                m_epgItemsPtr.push_back(itemsPointer);
                itemsPointer.start = i;
              }
              iLastChannelID = iCurrentChannelID;
              CGUIListItemPtr item(new CFileItem(*channel));
              m_channelItems.push_back(item);
            }
          }
          if (!m_programmeItems.empty())
          {
            itemsPointer.stop = m_programmeItems.size()-1;
            m_epgItemsPtr.push_back(itemsPointer);
          }

          ClearGridIndex();
          m_gridIndex.reserve(m_channelItems.size());
          for (unsigned int i = 0; i < m_channelItems.size(); i++)
          {
            std::vector<GridItemsPtr> blocks(MAXBLOCKS);
            m_gridIndex.push_back(blocks);
          }

          FreeItemsMemory();
          UpdateLayout();

          /* Create Ruler items */
          CDateTime ruler; ruler.SetFromUTCDateTime(m_gridStart);
          CDateTime rulerEnd; rulerEnd.SetFromUTCDateTime(m_gridEnd);
          CDateTimeSpan unit(0, 0, m_rulerUnit * MINSPERBLOCK, 0);
          CGUIListItemPtr rulerItem(new CFileItem(ruler.GetAsLocalizedDate(true, true)));
          rulerItem->SetProperty("DateLabel", true);
          m_rulerItems.push_back(rulerItem);

          for (; ruler < rulerEnd; ruler += unit)
          {
            CGUIListItemPtr rulerItem(new CFileItem(ruler.GetAsLocalizedTime("", false)));
            rulerItem->SetLabel2(ruler.GetAsLocalizedDate(true, true));
            m_rulerItems.push_back(rulerItem);
          }

          UpdateItems();
          return true;
        }
        break;

      case GUI_MSG_REFRESH_LIST:
        // update our list contents
        for (unsigned int i = 0; i < m_channelItems.size(); ++i)
          m_channelItems[i]->SetInvalid();
        for (unsigned int i = 0; i < m_programmeItems.size(); ++i)
          m_programmeItems[i]->SetInvalid();
        for (unsigned int i = 0; i < m_rulerItems.size(); ++i)
          m_rulerItems[i]->SetInvalid();
        break;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::UpdateItems()
{
  /* check for invalid start and end time */
  if (m_gridStart >= m_gridEnd)
  {
    CLog::Log(LOGERROR, "CGUIEPGGridContainer - %s - invalid start and end time set", __FUNCTION__);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), GetParentID()); // message the window
    SendWindowMessage(msg);
    return;
  }

  CDateTimeSpan gridDuration = m_gridEnd - m_gridStart;
  m_blocks = (gridDuration.GetDays()*24*60 + gridDuration.GetHours()*60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  if (m_blocks >= MAXBLOCKS)
    m_blocks = MAXBLOCKS;

  /* if less than one page, can't display grid */
  if (m_blocks < m_blocksPerPage)
  {
    CLog::Log(LOGERROR, "CGUIEPGGridContainer - %s - Less than one page of data available.", __FUNCTION__);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), GetParentID()); // message the window
    SendWindowMessage(msg);
    return;
  }

  CDateTimeSpan blockDuration;
  blockDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK, 0);

  long tick(XbmcThreads::SystemClockMillis());

  for (unsigned int row = 0; row < m_epgItemsPtr.size(); ++row)
  {
    CDateTime gridCursor  = m_gridStart; //reset cursor for new channel
    unsigned long progIdx = m_epgItemsPtr[row].start;
    unsigned long lastIdx = m_epgItemsPtr[row].stop;
    int iEpgId            = ((CFileItem *)m_programmeItems[progIdx].get())->GetEPGInfoTag()->EpgID();

    /** FOR EACH BLOCK **********************************************************************/

    for (int block = 0; block < m_blocks; block++)
    {
      while (progIdx <= lastIdx)
      {
        CGUIListItemPtr item = m_programmeItems[progIdx];
        const CEpgInfoTag* tag = ((CFileItem *)item.get())->GetEPGInfoTag();
        if (tag == NULL)
        {
          progIdx++;
          continue;
        }

        if (tag->EpgID() != iEpgId || gridCursor < tag->StartAsUTC() || m_gridEnd <= tag->StartAsUTC())
          break;

        if (gridCursor < tag->EndAsUTC())
        {
          m_gridIndex[row][block].item = item;
          break;
        }
        
        progIdx++;
      }

      gridCursor += blockDuration;
    }

    /** FOR EACH BLOCK **********************************************************************/
    int itemSize = 1; // size of the programme in blocks
    int savedBlock = 0;

    for (int block = 0; block < m_blocks; block++)
    {
      CGUIListItemPtr item = m_gridIndex[row][block].item;

      if (item != m_gridIndex[row][block+1].item)
      {
        if (!item)
        {
          CEpgInfoTag gapTag;
          CFileItemPtr gapItem(new CFileItem(gapTag));
          for (int i = block ; i > block - itemSize; i--)
          {
            m_gridIndex[row][i].item = gapItem;
          }
        }
        else
        {
          const CEpgInfoTag* tag = ((CFileItem *)item.get())->GetEPGInfoTag();
          m_gridIndex[row][savedBlock].item->SetProperty("GenreType", tag->GenreType());
        }

        m_gridIndex[row][savedBlock].originWidth = itemSize*m_blockSize;
        m_gridIndex[row][savedBlock].originHeight = m_channelHeight;

        m_gridIndex[row][savedBlock].width = m_gridIndex[row][savedBlock].originWidth;
        m_gridIndex[row][savedBlock].height = m_gridIndex[row][savedBlock].originHeight;

        itemSize = 1;
        savedBlock = block+1;
      }
      else
      {
        itemSize++;
      }
    }
  }

  /******************************************* END ******************************************/

  CLog::Log(LOGDEBUG, "CGUIEPGGridContainer - %s completed successfully in %u ms", __FUNCTION__, (unsigned int)(XbmcThreads::SystemClockMillis()-tick));

  m_channels = (int)m_epgItemsPtr.size();
  m_item = GetItem(m_channelCursor);
  if (m_item)
    SetBlock(GetBlock(m_item->item, m_channelCursor));

  SetInvalid();
  GoToNow();
}

void CGUIEPGGridContainer::ChannelScroll(int amount)
{
  // increase or decrease the vertical offset
  int offset = m_channelOffset + amount;

  if (offset > m_channels - m_channelsPerPage)
  {
    offset = m_channels - m_channelsPerPage;
  }

  if (offset < 0) offset = 0;

  ScrollToChannelOffset(offset);
}

void CGUIEPGGridContainer::ProgrammesScroll(int amount)
{
  // increase or decrease the horizontal offset
  ScrollToBlockOffset(m_blockOffset + amount);
}

void CGUIEPGGridContainer::OnUp()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_UP);
  if (m_channelCursor > 0)
  {
    SetChannel(m_channelCursor - 1);
  }
  else if (m_channelCursor == 0 && m_channelOffset)
  {
    ScrollToChannelOffset(m_channelOffset - 1);
    SetChannel(0);
  }
  else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
  {
    int offset = m_channels - m_channelsPerPage;

    if (offset < 0) offset = 0;

    SetChannel(m_channels - offset - 1);

    ScrollToChannelOffset(offset);
  }
  else
    CGUIControl::OnUp();
}

void CGUIEPGGridContainer::OnDown()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_DOWN);
  if (m_channelOffset + m_channelCursor + 1 < m_channels)
  {
    if (m_channelCursor + 1 < m_channelsPerPage)
    {
      SetChannel(m_channelCursor + 1);
    }
    else
    {
      ScrollToChannelOffset(m_channelOffset + 1);
      SetChannel(m_channelsPerPage - 1);
    }
  }
  else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
  {
    SetChannel(0);
    ScrollToChannelOffset(0);
  }
  else
    CGUIControl::OnDown();
}

void CGUIEPGGridContainer::OnLeft()
{
  if (!m_gridIndex.empty() && m_item)
  {
    if (m_channelCursor + m_channelOffset >= 0 && m_blockOffset >= 0 &&
        m_item->item != m_gridIndex[m_channelCursor + m_channelOffset][m_blockOffset].item)
    {
      // this is not first item on page
      m_item = GetPrevItem(m_channelCursor);
      SetBlock(GetBlock(m_item->item, m_channelCursor));

      return;
    }
    else if (m_blockCursor <= 0 && m_blockOffset && m_blockOffset - BLOCK_SCROLL_OFFSET >= 0)
    {
      // this is the first item on page
      ScrollToBlockOffset(m_blockOffset - BLOCK_SCROLL_OFFSET);
      SetBlock(GetBlock(m_item->item, m_channelCursor));

      return;
    }
  }

  CGUIControl::OnLeft();
}

void CGUIEPGGridContainer::OnRight()
{
  if (!m_gridIndex.empty() && m_item)
  {
    if (m_item->item != m_gridIndex[m_channelCursor + m_channelOffset][m_blocksPerPage + m_blockOffset - 1].item)
    {
      // this is not last item on page
      m_item = GetNextItem(m_channelCursor);
      SetBlock(GetBlock(m_item->item, m_channelCursor));

      return;
    }
    else if ((m_blockOffset != m_blocks - m_blocksPerPage) && m_blocks > m_blocksPerPage && m_blockOffset + BLOCK_SCROLL_OFFSET <= m_blocks)
    {
      // this is the last item on page
      ScrollToBlockOffset(m_blockOffset + BLOCK_SCROLL_OFFSET);
      SetBlock(GetBlock(m_item->item, m_channelCursor));

      return;
    }
  }

  CGUIControl::OnRight();
}

void CGUIEPGGridContainer::SetChannel(const std::string &channel)
{
  int iChannelIndex(-1);
  for (unsigned int iIndex = 0; iIndex < m_channelItems.size(); iIndex++)
  {
    std::string strPath = m_channelItems[iIndex]->GetProperty("path").asString();
    if (strPath == channel)
    {
      iChannelIndex = iIndex;
      break;
    }
  }

  SetSelectedChannel(iChannelIndex);
}

void CGUIEPGGridContainer::SetChannel(const CPVRChannel &channel)
{
  int iChannelIndex(-1);
  for (unsigned int iIndex = 0; iIndex < m_channelItems.size(); iIndex++)
  {
    int iChannelId = (int)m_channelItems[iIndex]->GetProperty("channelid").asInteger(-1);
    if (iChannelId == channel.ChannelID())
    {
      iChannelIndex = iIndex;
      break;
    }
  }

  SetSelectedChannel(iChannelIndex);
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  if (m_blockCursor + m_blockOffset == 0 || m_blockOffset + m_blockCursor + GetItemSize(m_item) == m_blocks)
  {
    m_item = GetItem(channel);
    if (m_item)
    {
      m_channelCursor = channel;
      SetBlock(GetBlock(m_item->item, channel));
    }
    return;
  }

  /* basic checks failed, need to correctly identify nearest item */
  m_item = GetClosestItem(channel);
  if (m_item)
  {
    m_channelCursor = channel;
    SetBlock(GetBlock(m_item->item, m_channelCursor));
  }
}

void CGUIEPGGridContainer::SetBlock(int block)
{
  if (block < 0)
    m_blockCursor = 0;
  else if (block > m_blocksPerPage - 1)
    m_blockCursor = m_blocksPerPage - 1;
  else
    m_blockCursor = block;
  m_item = GetItem(m_channelCursor);
}

CGUIListItemLayout *CGUIEPGGridContainer::GetFocusedLayout() const
{
  CGUIListItemPtr item = GetListItem(0);

  if (item.get()) return item->GetFocusedLayout();

  return NULL;
}

bool CGUIEPGGridContainer::SelectItemFromPoint(const CPoint &point, bool justGrid /* = false */)
{
  /* point has already had origin set to m_posX, m_posY */
  if (!m_focusedProgrammeLayout || !m_programmeLayout || (justGrid && point.x < 0))
    return false;

  int channel = (int)(point.y / m_channelHeight);
  int block   = (int)(point.x / m_blockSize);

  if (channel > m_channelsPerPage) channel = m_channelsPerPage - 1;
  if (channel >= m_channels) channel = m_channels - 1;
  if (channel < 0) channel = 0;
  if (block > m_blocksPerPage) block = m_blocksPerPage - 1;
  if (block < 0) block = 0;

  int channelIndex = channel + m_channelOffset;
  int blockIndex = block + m_blockOffset;

  // bail if out of range
  if (channelIndex >= m_channels || blockIndex >= m_blocks)
    return false;
  // bail if block isn't occupied
  if (!m_gridIndex[channelIndex][blockIndex].item)
    return false;

  SetChannel(channel);
  SetBlock(block);
  return true;
}

EVENT_RESULT CGUIEPGGridContainer::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  switch (event.m_id)
  {
  case ACTION_MOUSE_LEFT_CLICK:
    OnMouseClick(0, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_RIGHT_CLICK:
    OnMouseClick(1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_DOUBLE_CLICK:
    OnMouseDoubleClick(0, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_WHEEL_UP:
    OnMouseWheel(-1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_WHEEL_DOWN:
    OnMouseWheel(1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_GESTURE_BEGIN:
    {
      // we want exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
      return EVENT_RESULT_HANDLED;
    }
  case ACTION_GESTURE_END:
    {
      // we're done with exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
      ScrollToChannelOffset(MathUtils::round_int(m_channelScrollOffset / m_channelLayout->Size(VERTICAL)));
      ScrollToBlockOffset(MathUtils::round_int(m_programmeScrollOffset / m_blockSize));
      return EVENT_RESULT_HANDLED;
    }
  case ACTION_GESTURE_PAN:
    {
      m_programmeScrollOffset -= event.m_offsetX;
      m_channelScrollOffset -= event.m_offsetY;

      m_channelOffset = MathUtils::round_int(m_channelScrollOffset / m_channelLayout->Size(VERTICAL));
      m_blockOffset = MathUtils::round_int(m_programmeScrollOffset / m_blockSize);
      ValidateOffset();
      return EVENT_RESULT_HANDLED;
    }
  default:
    return EVENT_RESULT_UNHANDLED;
  }
}

bool CGUIEPGGridContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_gridPosX, m_posY + m_rulerHeight), false);
  return CGUIControl::OnMouseOver(point);
}

bool CGUIEPGGridContainer::OnMouseClick(int dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_posY + m_rulerHeight)))
  { // send click message to window
    OnClick(ACTION_MOUSE_LEFT_CLICK + dwButton);
    return true;
  }

  return false;
}

bool CGUIEPGGridContainer::OnMouseDoubleClick(int dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_posY + m_rulerHeight)))
  { // send double click message to window
    OnClick(ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }

  return false;
}

bool CGUIEPGGridContainer::OnClick(int actionID)
{
  int subItem = 0;

  if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
  {
    // grab the currently focused subitem (if applicable)
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();

    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }

  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  return SendWindowMessage(msg);
}

bool CGUIEPGGridContainer::OnMouseWheel(char wheel, const CPoint &point)
{
  ///doesn't work while an item is selected?
  ProgrammesScroll(-wheel);
  return true;
}

CPVRChannel* CGUIEPGGridContainer::GetChannel(int iIndex)
{
  if (iIndex >= 0 && (size_t) iIndex < m_channelItems.size())
  {
    CFileItemPtr fileItem = std::static_pointer_cast<CFileItem>(m_channelItems[iIndex]);
    if (fileItem->HasPVRChannelInfoTag())
      return fileItem->GetPVRChannelInfoTag();
  }
  
  return NULL;
}

void CGUIEPGGridContainer::SetSelectedChannel(int channelIndex)
{
  if (channelIndex < 0)
    return;
  
  if (channelIndex - m_channelOffset <= 0)
  {
    ScrollToChannelOffset(0);
    SetChannel(channelIndex);
  }
  else if (channelIndex - m_channelOffset < m_channelsPerPage && channelIndex - m_channelOffset >= 0)
  {
    SetChannel(channelIndex - m_channelOffset);
  }
  else if(channelIndex < m_channels - m_channelsPerPage)
  {
    ScrollToChannelOffset(channelIndex - m_channelsPerPage + 1);
    SetChannel(m_channelsPerPage - 1);
  }
  else
  {
    ScrollToChannelOffset(m_channels - m_channelsPerPage);
    SetChannel(channelIndex - (m_channels - m_channelsPerPage));
  }
}

int CGUIEPGGridContainer::GetSelectedItem() const
{
  if (m_gridIndex.empty() ||
      m_epgItemsPtr.empty() ||
      m_channelCursor + m_channelOffset >= m_channels ||
      m_blockCursor + m_blockOffset >= m_blocks)
    return -1;

  CGUIListItemPtr currentItem = m_gridIndex[m_channelCursor + m_channelOffset][m_blockCursor + m_blockOffset].item;
  if (!currentItem)
    return -1;

  for (int i = 0; i < (int)m_programmeItems.size(); i++)
  {
    if (currentItem == m_programmeItems[i])
      return i;
  }
  return -1;
}

CGUIListItemPtr CGUIEPGGridContainer::GetListItem(int offset, unsigned int flag) const
{
  if (m_channelItems.empty())
    return CGUIListItemPtr();

  int item = m_channelCursor + m_channelOffset + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION)
    item = (int)(m_channelScrollOffset / m_channelLayout->Size(VERTICAL));

  if (flag & INFOFLAG_LISTITEM_WRAP)
  {
    item %= (int)m_channelItems.size();
    if (item < 0) item += m_channelItems.size();
    return m_channelItems[item];
  }
  else
  {
    if (item >= 0 && item < (int)m_channelItems.size())
      return m_channelItems[item];
  }
  return CGUIListItemPtr();
}

std::string CGUIEPGGridContainer::GetLabel(int info) const
{
  std::string label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    label = StringUtils::Format("%u", (m_channels + m_channelsPerPage - 1) / m_channelsPerPage);
    break;
  case CONTAINER_CURRENT_PAGE:
    label = StringUtils::Format("%u", 1 + (m_channelCursor + m_channelOffset) / m_channelsPerPage );
    break;
  case CONTAINER_POSITION:
    label = StringUtils::Format("%i", 1 + m_channelCursor + m_channelOffset);
    break;
  case CONTAINER_NUM_ITEMS:
    label = StringUtils::Format("%u", m_channels);
    break;
  default:
      break;
  }
  return label;
}

GridItemsPtr *CGUIEPGGridContainer::GetClosestItem(const int &channel)
{
  GridItemsPtr *closest = GetItem(channel);

  if (!closest)
    return NULL;

  int block = GetBlock(closest->item, channel);
  int left;   // num blocks to start of previous item
  int right;  // num blocks to start of next item

  if (block == m_blockCursor)
    return closest; // item & m_item start together

  if (block + GetItemSize(closest) == m_blockCursor + GetItemSize(m_item))
    return closest; // closest item ends when current does

  if (block > m_blockCursor)  // item starts after m_item
  {
    left = m_blockCursor - GetBlock(closest->item, channel);
    right = block - m_blockCursor;
  }
  else
  {
    left  = m_blockCursor - block;
    right = GetBlock(GetNextItem(channel)->item, channel) - m_blockCursor;
  }

  if (right <= SHORTGAP && right <= left && m_blockCursor + right < m_blocksPerPage)
    return &m_gridIndex[channel + m_channelOffset][m_blockCursor + right + m_blockOffset];

  return &m_gridIndex[channel + m_channelOffset][m_blockCursor - left  + m_blockOffset];
}

int CGUIEPGGridContainer::GetItemSize(GridItemsPtr *item)
{
  if (!item)
    return (int) m_blockSize; /// stops it crashing

  return (int) (item->width / m_blockSize);
}

int CGUIEPGGridContainer::GetBlock(const CGUIListItemPtr &item, const int &channel)
{
  if (!item)
    return 0;

  return GetRealBlock(item, channel) - m_blockOffset;
}

int CGUIEPGGridContainer::GetRealBlock(const CGUIListItemPtr &item, const int &channel)
{
  int channelIndex = channel + m_channelOffset;
  int block = 0;

  while (m_gridIndex[channelIndex][block].item != item && block < m_blocks)
    block++;

  return block;
}

GridItemsPtr *CGUIEPGGridContainer::GetNextItem(const int &channel)
{
  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_channels || blockIndex >= m_blocks)
    return NULL;

  int i = m_blockCursor;

  while (i < m_blocksPerPage && m_gridIndex[channelIndex][i + m_blockOffset].item == m_gridIndex[channelIndex][blockIndex].item)
    i++;

  return &m_gridIndex[channelIndex][i + m_blockOffset];
}

GridItemsPtr *CGUIEPGGridContainer::GetPrevItem(const int &channel)
{
  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_channels || blockIndex >= m_blocks)
    return NULL;

  int i = m_blockCursor;

  while (i > 0 && m_gridIndex[channelIndex][i + m_blockOffset].item == m_gridIndex[channelIndex][blockIndex].item)
    i--;

  return &m_gridIndex[channelIndex][i + m_blockOffset];
}

GridItemsPtr *CGUIEPGGridContainer::GetItem(const int &channel)
{
  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_channels || blockIndex >= m_blocks)
    return NULL;

  return &m_gridIndex[channelIndex][blockIndex];
}

void CGUIEPGGridContainer::SetFocus(bool focus)
{
  if (focus != HasFocus())
    SetInvalid();
  CGUIControl::SetFocus(focus);
}

void CGUIEPGGridContainer::DoRender()
{
  CGUIControl::DoRender();
  m_wasReset = false;
}

void CGUIEPGGridContainer::ScrollToChannelOffset(int offset)
{
  float size = m_programmeLayout->Size(VERTICAL);
  int range = m_channelsPerPage / 4;

  if (range <= 0) range = 1;

  if (offset * size < m_channelScrollOffset &&  m_channelScrollOffset - offset * size > size * range)
  { // scrolling up, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_channelScrollOffset && offset * size - m_channelScrollOffset > size * range)
  { // scrolling down, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset - range) * size;
  }

  m_channelScrollSpeed = (offset * size - m_channelScrollOffset) / m_scrollTime;

  m_channelOffset = offset;
}

void CGUIEPGGridContainer::ScrollToBlockOffset(int offset)
{
  // make sure offset is in valid range
  offset = std::max(0, std::min(offset, m_blocks - m_blocksPerPage));

  float size = m_blockSize;
  int range = m_blocksPerPage / 1;

  if (range <= 0) range = 1;

  if (offset * size < m_programmeScrollOffset &&  m_programmeScrollOffset - offset * size > size * range)
  { // scrolling left, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_programmeScrollOffset && offset * size - m_programmeScrollOffset > size * range)
  { // scrolling right, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset - range) * size;
  }

  m_programmeScrollSpeed = (offset * size - m_programmeScrollOffset) / m_scrollTime;

  m_blockOffset = offset;
}

void CGUIEPGGridContainer::ValidateOffset()
{
  if (!m_programmeLayout)
    return;

  if (m_channelOffset > m_channels - m_channelsPerPage || m_channelScrollOffset > (m_channels - m_channelsPerPage) * m_channelHeight)
  {
    m_channelOffset = m_channels - m_channelsPerPage;
    m_channelScrollOffset = m_channelOffset * m_channelHeight;
  }

  if (m_channelOffset < 0 || m_channelScrollOffset < 0)
  {
    m_channelOffset = 0;
    m_channelScrollOffset = 0;
  }

  if (m_blockOffset > m_blocks - m_blocksPerPage || m_programmeScrollOffset > (m_blocks - m_blocksPerPage) * m_blockSize)
  {
    m_blockOffset = m_blocks - m_blocksPerPage;
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
  }

  if (m_blockOffset < 0 || m_programmeScrollOffset < 0)
  {
    m_blockOffset = 0;
    m_programmeScrollOffset = 0;
  }
}

void CGUIEPGGridContainer::LoadLayout(TiXmlElement *layout)
{
  /* layouts for the channel column */
  TiXmlElement *itemElement = layout->FirstChildElement("channellayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false);
    m_channelLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("channellayout");
  }
  itemElement = layout->FirstChildElement("focusedchannellayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), true);
    m_focusedChannelLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedchannellayout");
  }

  /* layouts for the grid items */
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), true);
    m_focusedProgrammeLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
  itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false);
    m_programmeLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }

  /* layout for the timeline above the grid */
  itemElement = layout->FirstChildElement("rulerlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false);
    m_rulerLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("rulerlayout");
  }
}

std::string CGUIEPGGridContainer::GetDescription() const
{
  std::string strLabel;
  int item = GetSelectedItem();
  if (item >= 0 && item < (int)m_programmeItems.size())
  {
    CGUIListItemPtr pItem = m_programmeItems[item];
    strLabel = pItem->GetLabel();
  }
  return strLabel;
}

void CGUIEPGGridContainer::ClearGridIndex(void)
{
  for (unsigned int i = 0; i < m_gridIndex.size(); i++)
  {
    for (int block = 0; block < m_blocks; block++)
    {
      if (m_gridIndex[i][block].item)
        m_gridIndex[i][block].item.get()->ClearProperties();
    }
    m_gridIndex[i].clear();
  }
  m_gridIndex.clear();
}

void CGUIEPGGridContainer::Reset()
{
  ClearGridIndex();

  m_wasReset = true;
  m_channelItems.clear();
  m_programmeItems.clear();
  m_rulerItems.clear();
  m_epgItemsPtr.clear();

  m_lastItem    = NULL;
  m_lastChannel = NULL;
}

void CGUIEPGGridContainer::GoToBegin()
{
  ScrollToBlockOffset(0);
  SetBlock(0);
}

void CGUIEPGGridContainer::GoToEnd()
{
  int blocksEnd = 0;   // the end block of the last epg element for the selected channel
  int blocksStart = 0; // the start block of the last epg element for the selected channel
  int blockOffset = 0; // the block offset to scroll to
  for (int blockIndex = m_blocks; blockIndex >= 0 && (!blocksEnd || !blocksStart); blockIndex--)
  {
    if (!blocksEnd && m_gridIndex[m_channelCursor + m_channelOffset][blockIndex].item != NULL)
      blocksEnd = blockIndex;
    if (blocksEnd && m_gridIndex[m_channelCursor + m_channelOffset][blocksEnd].item != 
                     m_gridIndex[m_channelCursor + m_channelOffset][blockIndex].item)
      blocksStart = blockIndex + 1;
  }
  if (blocksEnd - blocksStart > m_blocksPerPage)
    blockOffset = blocksStart;
  else if (blocksEnd > m_blocksPerPage)
    blockOffset = blocksEnd - m_blocksPerPage;

  ScrollToBlockOffset(blockOffset); // scroll to the start point of the last epg element
  SetBlock(m_blocksPerPage - 1);    // select the last epg element
}

void CGUIEPGGridContainer::GoToNow()
{
  CDateTime currentDate = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
  int offset = ((currentDate - m_gridStart).GetSecondsTotal() / 60 - 30) / MINSPERBLOCK;
  ScrollToBlockOffset(offset);
}

void CGUIEPGGridContainer::SetStartEnd(CDateTime start, CDateTime end)
{
  m_gridStart = CDateTime(start.GetYear(), start.GetMonth(), start.GetDay(), start.GetHour(), start.GetMinute() >= 30 ? 30 : 0, 0);
  m_gridEnd = CDateTime(end.GetYear(), end.GetMonth(), end.GetDay(), end.GetHour(), end.GetMinute() >= 30 ? 30 : 0, 0);

  CLog::Log(LOGDEBUG, "CGUIEPGGridContainer - %s - start=%s end=%s",
      __FUNCTION__, m_gridStart.GetAsLocalizedDateTime(false, true).c_str(), m_gridEnd.GetAsLocalizedDateTime(false, true).c_str());
}

void CGUIEPGGridContainer::UpdateLayout()
{
  CGUIListItemLayout *oldFocusedChannelLayout   = m_focusedChannelLayout;
  CGUIListItemLayout *oldChannelLayout          = m_channelLayout;
  CGUIListItemLayout *oldFocusedProgrammeLayout = m_focusedProgrammeLayout;
  CGUIListItemLayout *oldProgrammeLayout        = m_programmeLayout;
  CGUIListItemLayout *oldRulerLayout            = m_rulerLayout;
  GetCurrentLayouts();

  if (!m_focusedProgrammeLayout || !m_programmeLayout || !m_focusedChannelLayout || !m_channelLayout || !m_rulerLayout)
    return;

  if (oldChannelLayout   == m_channelLayout   && oldFocusedChannelLayout   == m_focusedChannelLayout   &&
      oldProgrammeLayout == m_programmeLayout && oldFocusedProgrammeLayout == m_focusedProgrammeLayout &&
      oldRulerLayout     == m_rulerLayout)
    return; // nothing has changed, so don't update stuff

  m_channelHeight     = m_channelLayout->Size(VERTICAL);
  m_channelWidth      = m_channelLayout->Size(HORIZONTAL);
  m_rulerHeight       = m_rulerLayout->Size(VERTICAL);
  m_gridPosX          = m_posX + m_channelWidth;
  m_gridPosY          = m_posY + m_rulerHeight;
  m_gridWidth         = m_width - m_channelWidth;
  m_gridHeight        = m_height - m_rulerHeight;
  m_blockSize         = m_gridWidth / m_blocksPerPage;
  m_rulerWidth        = m_rulerUnit * m_blockSize;
  m_channelPosX       = m_posX;
  m_channelPosY       = m_posY + m_rulerHeight;
  m_rulerPosX         = m_posX + m_channelWidth;
  m_rulerPosY         = m_posY;
  m_channelsPerPage   = (int)(m_gridHeight / m_channelHeight);
  m_programmesPerPage = (int)(m_gridWidth / m_blockSize) + 1;

  // ensure that the scroll offsets are a multiple of our sizes
  m_channelScrollOffset   = m_channelOffset * m_programmeLayout->Size(VERTICAL);
  m_programmeScrollOffset = m_blockOffset * m_blockSize;
}

void CGUIEPGGridContainer::UpdateScrollOffset(unsigned int currentTime)
{
  if (!m_programmeLayout)
    return;
  m_channelScrollOffset += m_channelScrollSpeed * (currentTime - m_channelScrollLastTime);
  if ((m_channelScrollSpeed < 0 && m_channelScrollOffset < m_channelOffset * m_programmeLayout->Size(VERTICAL)) ||
      (m_channelScrollSpeed > 0 && m_channelScrollOffset > m_channelOffset * m_programmeLayout->Size(VERTICAL)))
  {
    m_channelScrollOffset = m_channelOffset * m_programmeLayout->Size(VERTICAL);
    m_channelScrollSpeed = 0;
  }
  m_channelScrollLastTime = currentTime;

  m_programmeScrollOffset += m_programmeScrollSpeed * (currentTime - m_programmeScrollLastTime);
  if ((m_programmeScrollSpeed < 0 && m_programmeScrollOffset < m_blockOffset * m_blockSize) ||
      (m_programmeScrollSpeed > 0 && m_programmeScrollOffset > m_blockOffset * m_blockSize))
  {
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
    m_programmeScrollSpeed = 0;
  }
  m_programmeScrollLastTime = currentTime;
}

void CGUIEPGGridContainer::GetCurrentLayouts()
{
  m_channelLayout = NULL;
  for (unsigned int i = 0; i < m_channelLayouts.size(); i++)
  {
    if (m_channelLayouts[i].CheckCondition())
    {
      m_channelLayout = &m_channelLayouts[i];
      break;
    }
  }
  if (!m_channelLayout && !m_channelLayouts.empty())
    m_channelLayout = &m_channelLayouts[0];  // failsafe

  m_focusedChannelLayout = NULL;
  for (unsigned int i = 0; i < m_focusedChannelLayouts.size(); i++)
  {
    if (m_focusedChannelLayouts[i].CheckCondition())
    {
      m_focusedChannelLayout = &m_focusedChannelLayouts[i];
      break;
    }
  }
  if (!m_focusedChannelLayout && !m_focusedChannelLayouts.empty())
    m_focusedChannelLayout = &m_focusedChannelLayouts[0];  // failsafe

  m_programmeLayout = NULL;
  for (unsigned int i = 0; i < m_programmeLayouts.size(); i++)
  {
    if (m_programmeLayouts[i].CheckCondition())
    {
      m_programmeLayout = &m_programmeLayouts[i];
      break;
    }
  }
  if (!m_programmeLayout && !m_programmeLayouts.empty())
    m_programmeLayout = &m_programmeLayouts[0];  // failsafe

  m_focusedProgrammeLayout = NULL;
  for (unsigned int i = 0; i < m_focusedProgrammeLayouts.size(); i++)
  {
    if (m_focusedProgrammeLayouts[i].CheckCondition())
    {
      m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[i];
      break;
    }
  }
  if (!m_focusedProgrammeLayout && !m_focusedProgrammeLayouts.empty())
    m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[0];  // failsafe

  m_rulerLayout = NULL;
  for (unsigned int i = 0; i < m_rulerLayouts.size(); i++)
  {
    if (m_rulerLayouts[i].CheckCondition())
    {
      m_rulerLayout = &m_rulerLayouts[i];
      break;
    }
  }
  if (!m_rulerLayout && !m_rulerLayouts.empty())
    m_rulerLayout = &m_rulerLayouts[0];  // failsafe
}

void CGUIEPGGridContainer::SetRenderOffset(const CPoint &offset)
{
  m_renderOffset = offset;
}

void CGUIEPGGridContainer::FreeItemsMemory()
{
  // free memory of items
  for (std::vector<CGUIListItemPtr>::iterator it = m_channelItems.begin(); it != m_channelItems.end(); it++)
    (*it)->FreeMemory();
  for (std::vector<CGUIListItemPtr>::iterator it = m_rulerItems.begin(); it != m_rulerItems.end(); it++)
    (*it)->FreeMemory();
  for (std::vector<CGUIListItemPtr>::iterator it = m_programmeItems.begin(); it != m_programmeItems.end(); it++)
    (*it)->FreeMemory();
}

void CGUIEPGGridContainer::FreeChannelMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    for (int i = 0; i < keepStart && i < (int)m_channelItems.size(); ++i)
      m_channelItems[i]->FreeMemory();
    for (int i = keepEnd + 1; i < (int)m_channelItems.size(); ++i)
      m_channelItems[i]->FreeMemory();
  }
  else
  { // wrapping
    for (int i = keepEnd + 1; i < keepStart && i < (int)m_channelItems.size(); ++i)
      m_channelItems[i]->FreeMemory();
  }
}

void CGUIEPGGridContainer::FreeProgrammeMemory(int channel, int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    if (keepStart > 0 && keepStart < m_blocks)
    {
      // if item exist and block is not part of visible item
      CGUIListItemPtr last = m_gridIndex[channel][keepStart].item;
      for (int i = keepStart - 1 ; i > 0 ; i--)
      {
        if (m_gridIndex[channel][i].item && m_gridIndex[channel][i].item != last)
        {
          m_gridIndex[channel][i].item->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that ocupy few blocks in a row
          last = m_gridIndex[channel][i].item;
        }
      }
    }

    if (keepEnd > 0 && keepEnd < m_blocks)
    {
      CGUIListItemPtr last = m_gridIndex[channel][keepEnd].item;
      for (int i = keepEnd + 1 ; i < m_blocks ; i++)
      {
        // if item exist and block is not part of visible item
        if (m_gridIndex[channel][i].item && m_gridIndex[channel][i].item != last)
        {
          m_gridIndex[channel][i].item->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that ocupy few blocks in a row
          last = m_gridIndex[channel][i].item;
        }
      }
    }
  }
}

void CGUIEPGGridContainer::FreeRulerMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    for (int i = 1; i < keepStart && i < (int)m_rulerItems.size(); ++i)
      m_rulerItems[i]->FreeMemory();
    for (int i = keepEnd + 1; i < (int)m_rulerItems.size(); ++i)
      m_rulerItems[i]->FreeMemory();
  }
  else
  { // wrapping
    for (int i = keepEnd + 1; i < keepStart && i < (int)m_rulerItems.size(); ++i)
    {
      if (i == 0)
        continue;
      m_rulerItems[i]->FreeMemory();
    }
  }
}

void CGUIEPGGridContainer::GetChannelCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_channelScrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheChannelItems;
  }
  else if (m_channelScrollSpeed < 0)
  {
    cacheBefore = m_cacheChannelItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheChannelItems / 2;
    cacheAfter = m_cacheChannelItems / 2;
  }
}

void CGUIEPGGridContainer::GetProgrammeCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_programmeScrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheProgrammeItems;
  }
  else if (m_programmeScrollSpeed < 0)
  {
    cacheBefore = m_cacheProgrammeItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheProgrammeItems / 2;
    cacheAfter = m_cacheProgrammeItems / 2;
  }
}
