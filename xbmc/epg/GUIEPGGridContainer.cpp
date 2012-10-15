/*
*      Copyright (C) 2012 Team XBMC
*      http://www.xbmc.org
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
#include "guilib/GUIFontManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/DirtyRegion.h"
#include <tinyxml.h>
#include "utils/log.h"
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

CGUIEPGGridContainer::CGUIEPGGridContainer(int parentID, int controlID, float posX, float posY, float width,
                                           float height, ORIENTATION orientation, int scrollTime,
                                           int preloadItems, int timeBlocks, int rulerUnit)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
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
  m_scrollTime            = scrollTime ? scrollTime : 1;
  m_renderTime            = 0;
  m_item                  = NULL;
  m_lastItem              = NULL;
  m_lastChannel           = NULL;
  m_orientation           = orientation;
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
  m_gridIndex             = NULL;
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
  Reset();
}

void CGUIEPGGridContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;
  m_renderTime = currentTime;

  changed = true;

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIEPGGridContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_focusedChannelLayout || !m_channelLayout || !m_rulerLayout || !m_focusedProgrammeLayout || !m_programmeLayout || m_rulerItems.size()<=1 || (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0))
    return;

  UpdateScrollOffset();

  int chanOffset  = (int)floorf(m_channelScrollOffset / m_programmeLayout->Size(m_orientation));
  int blockOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);
  int rulerOffset = (int)floorf(m_programmeScrollOffset / m_blockSize);

  /// Render channel names
  int cacheBeforeChannel, cacheAfterChannel;
  GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

  // Free memory not used on screen
  if ((int)m_channelItems.size() > m_channelsPerPage + cacheBeforeChannel + cacheAfterChannel)
    FreeChannelMemory(CorrectOffset(chanOffset - cacheBeforeChannel, 0), CorrectOffset(chanOffset + m_channelsPerPage + 1 + cacheAfterChannel, 0));

  if (m_orientation == VERTICAL)
    g_graphicsContext.SetClipRegion(m_channelPosX, m_channelPosY, m_channelWidth, m_gridHeight);
  else
    g_graphicsContext.SetClipRegion(m_channelPosX, m_channelPosY, m_gridWidth, m_channelHeight);

  CPoint originChannel = CPoint(m_channelPosX, m_channelPosY) + m_renderOffset;
  float pos = (m_orientation == VERTICAL) ? originChannel.y : originChannel.x;
  float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (chanOffset - cacheBeforeChannel) * m_channelLayout->Size(m_orientation) - m_channelScrollOffset;
  if (m_channelOffset + m_channelCursor < chanOffset)
    drawOffset += m_focusedChannelLayout->Size(m_orientation) - m_channelLayout->Size(m_orientation);
  pos += drawOffset;
  end += cacheAfterChannel * m_channelLayout->Size(m_orientation);

  float focusedPos = 0;
  CGUIListItemPtr focusedItem;
  int current = chanOffset;// - cacheBeforeChannel;
  while (pos < end && (int)m_channelItems.size())
  {
    int itemNo = CorrectOffset(current, 0);
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
        if (m_orientation == VERTICAL)
          RenderChannelItem(originChannel.x, pos, item.get(), false);
        else
          RenderChannelItem(pos, originChannel.y, item.get(), false);
      }
    }
    // increment our position
    pos += focused ? m_focusedChannelLayout->Size(m_orientation) : m_channelLayout->Size(m_orientation);
    current++;
  }
  // render focused item last so it can overlap other items
  if (focusedItem)
  {
    if (m_orientation == VERTICAL)
      RenderChannelItem(originChannel.x, focusedPos, focusedItem.get(), true);
    else
      RenderChannelItem(focusedPos, originChannel.y, focusedItem.get(), true);
  }
  g_graphicsContext.RestoreClipRegion();

  /// Render the ruler items
  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  CGUIListItemPtr item = m_rulerItems[0];
  g_graphicsContext.SetOrigin(m_posX, m_posY);
  item->SetLabel(m_rulerItems[rulerOffset/m_rulerUnit+1]->GetLabel2());
  if (!item->GetLayout())
  {
    CGUIListItemLayout *layout = new CGUIListItemLayout(*m_rulerLayout);
    if (m_orientation == VERTICAL)
      layout->SetWidth(m_channelWidth);
    else
      layout->SetHeight(m_channelHeight);
    item->SetLayout(layout);
  }
  if (item->GetLayout())
  {
    CDirtyRegionList dirtyRegions;
    item->GetLayout()->Process(item.get(),m_parentID,m_renderTime,dirtyRegions);
    item->GetLayout()->Render(item.get(), m_parentID);
  }
  g_graphicsContext.RestoreOrigin();

  int cacheBeforeRuler, cacheAfterRuler;
  GetRulerCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

  g_graphicsContext.RestoreClipRegion();

  // Free memory not used on screen
  if ((int)m_rulerItems.size() > m_blocksPerPage + cacheBeforeRuler + cacheAfterRuler)
    FreeRulerMemory(CorrectOffset(rulerOffset - cacheBeforeRuler, 0), CorrectOffset(rulerOffset + m_blocksPerPage + 1 + cacheAfterRuler, 0));

  if (m_orientation == VERTICAL)
    g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_gridWidth, m_rulerHeight);
  else
    g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_rulerWidth, m_gridHeight);

  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  pos = (m_orientation == VERTICAL) ? originRuler.x : originRuler.y;
  end = (m_orientation == VERTICAL) ? m_posX + m_width : m_posY + m_height;
  drawOffset = (rulerOffset - cacheBeforeRuler) * m_blockSize - m_programmeScrollOffset;
  pos += drawOffset;
  end += cacheAfterRuler * m_rulerLayout->Size(m_orientation == VERTICAL ? HORIZONTAL : VERTICAL);

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
    if (m_orientation == VERTICAL)
    {
      g_graphicsContext.SetOrigin(pos, originRuler.y);
      pos += m_rulerWidth;
    }
    else
    {
      g_graphicsContext.SetOrigin(originRuler.x, pos);
      pos += m_rulerHeight;
    }
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_rulerLayout);
      if (m_orientation == VERTICAL)
        layout->SetWidth(m_rulerWidth);
      else
        layout->SetHeight(m_rulerHeight);

      item->SetLayout(layout);
    }
    if (item->GetLayout())
    {
      CDirtyRegionList dirtyRegions;
      item->GetLayout()->Process(item.get(),m_parentID,m_renderTime,dirtyRegions);
      item->GetLayout()->Render(item.get(), m_parentID);
    }
    g_graphicsContext.RestoreOrigin();

    rulerOffset += m_rulerUnit;
  }
  g_graphicsContext.RestoreClipRegion();

  /// Render programmes
  int cacheBeforeProgramme, cacheAfterProgramme;
  GetProgrammeCacheOffsets(cacheBeforeProgramme, cacheAfterProgramme);

  // Free memory not used on screen
  if ((int)m_programmeItems.size() > m_ProgrammesPerPage + cacheBeforeProgramme + cacheAfterProgramme)
    FreeProgrammeMemory(CorrectOffset(blockOffset - cacheBeforeProgramme, 0), CorrectOffset(blockOffset + m_ProgrammesPerPage + 1 + cacheAfterProgramme, 0));

  g_graphicsContext.SetClipRegion(m_gridPosX, m_gridPosY, m_gridWidth, m_gridHeight);
  CPoint originProgramme = CPoint(m_gridPosX, m_gridPosY) + m_renderOffset;
  float posA = (m_orientation != VERTICAL) ? originProgramme.y : originProgramme.x;
  float endA = (m_orientation != VERTICAL) ? m_posY + m_height : m_posX + m_width;
  float posB = (m_orientation == VERTICAL) ? originProgramme.y : originProgramme.x;
  float endB = (m_orientation == VERTICAL) ? m_gridPosY + m_gridHeight : m_posX + m_width;
  endA += cacheAfterProgramme * m_blockSize;

  float DrawOffsetA = blockOffset * m_blockSize - m_programmeScrollOffset;
  posA += DrawOffsetA;
  float DrawOffsetB = (chanOffset - cacheBeforeProgramme) * m_channelLayout->Size(m_orientation) - m_channelScrollOffset;
  posB += DrawOffsetB;

  int channel = chanOffset;

  float focusedPosX = 0;
  float focusedPosY = 0;
  float focusedwidth = 0;
  float focusedheight = 0;
  while (posB < endB && m_channelItems.size())
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
      while (m_gridIndex[channel][startBlock].item == item)
        startBlock--;

      block = startBlock + 1;
      int missingSection = blockOffset - block;
      posA2 -= missingSection * m_blockSize;
    }

    while (posA2 < endA && m_programmeItems.size())   // FOR EACH ITEM ///////////////
    {
      item = m_gridIndex[channel][block].item;
      if (!item || !item.get()->IsFileItem())
        break;

      bool focused = (channel == m_channelOffset + m_channelCursor) && (item == m_gridIndex[m_channelOffset + m_channelCursor][m_blockOffset + m_blockCursor].item);

      // render our item
      if (focused)
      {
        if (m_orientation == VERTICAL)
        {
          focusedPosX = posA2;
          focusedPosY = posB;
        }
        else
        {
          focusedPosX = posB;
          focusedPosY = posA2;
        }
        focusedItem = item;
        focusedwidth = m_gridIndex[channel][block].width;
        focusedheight = m_gridIndex[channel][block].height;
      }
      else
      {
        if (m_orientation == VERTICAL)
          RenderProgrammeItem(posA2, posB, m_gridIndex[channel][block].width, m_gridIndex[channel][block].height, item.get(), focused);
        else
          RenderProgrammeItem(posB, posA2, m_gridIndex[channel][block].width, m_gridIndex[channel][block].height, item.get(), focused);
      }

      // increment our X position
      if (m_orientation == VERTICAL)
      {
        posA2 += m_gridIndex[channel][block].width; // assumes focused & unfocused layouts have equal length
        block += (int)(m_gridIndex[channel][block].width / m_blockSize);
      }
      else
      {
        posA2 += m_gridIndex[channel][block].height; // assumes focused & unfocused layouts have equal length
        block += (int)(m_gridIndex[channel][block].height / m_blockSize);
      }
    }

    // increment our Y position
    channel++;
    posB += m_orientation == VERTICAL ? m_channelHeight : m_channelWidth;
  }

  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderProgrammeItem(focusedPosX, focusedPosY, focusedwidth, focusedheight, focusedItem.get(), true);

  g_graphicsContext.RestoreClipRegion();

  CGUIControl::Render();
}

void CGUIEPGGridContainer::RenderChannelItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  if (!m_focusedChannelLayout || !m_channelLayout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedChannelLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastChannel || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastChannel && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);
        unsigned int subItem = 1;
        if (m_lastChannel && m_lastChannel->GetFocusedLayout())
          subItem = m_lastChannel->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      CDirtyRegionList dirtyRegions;
      item->GetFocusedLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetFocusedLayout()->Render(item, m_parentID);
    }
    m_lastChannel = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);  // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_channelLayout);
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
    {
      CDirtyRegionList dirtyRegions;
      item->GetFocusedLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetFocusedLayout()->Render(item, m_parentID);
    }
    else if (item->GetLayout())
    {
      CDirtyRegionList dirtyRegions;
      item->GetLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetLayout()->Render(item, m_parentID);
    }
  }
  g_graphicsContext.RestoreOrigin();
}

void CGUIEPGGridContainer::RenderProgrammeItem(float posX, float posY, float width, float height, CGUIListItem *item, bool focused)
{
  if (!m_focusedProgrammeLayout || !m_programmeLayout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedProgrammeLayout);
      CFileItem *fileItem = item->IsFileItem() ? (CFileItem *)item : NULL;
      if (fileItem)
      {
        const CEpgInfoTag* tag = fileItem->GetEPGInfoTag();
        if (m_orientation == VERTICAL)
          layout->SetWidth(width);
        else
          layout->SetHeight(height);

        item->SetProperty("GenreType", tag->GenreType());
      }
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastItem && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);
        unsigned int subItem = 1;
        if (m_lastItem && m_lastItem->GetFocusedLayout())
          subItem = m_lastItem->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      CDirtyRegionList dirtyRegions;
      item->GetFocusedLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetFocusedLayout()->Render(item, m_parentID);
    }
    m_lastItem = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);  // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_programmeLayout);
      CFileItem *fileItem = item->IsFileItem() ? (CFileItem *)item : NULL;
      if (fileItem)
      {
        const CEpgInfoTag* tag = fileItem->GetEPGInfoTag();
        if (m_orientation == VERTICAL)
          layout->SetWidth(width);
        else
          layout->SetHeight(height);

        item->SetProperty("GenreType", tag->GenreType());
      }
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
    {
      CDirtyRegionList dirtyRegions;
      item->GetFocusedLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetFocusedLayout()->Render(item, m_parentID);
    }
    else if (item->GetLayout())
    {
      CDirtyRegionList dirtyRegions;
      item->GetLayout()->Process(item,m_parentID,m_renderTime,dirtyRegions);
      item->GetLayout()->Render(item, m_parentID);
    }
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
    { // use base class implementation

      return CGUIControl::OnAction(action);
    }

    break;
  case ACTION_PAGE_UP:
    {
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == 0)
        { // already on the first page, so move to the first item
          SetChannel(0);
        }
        else
        { // scroll up to the previous page
          ChannelScroll(-m_channelsPerPage);
        }
      }
      else
        ProgrammesScroll(-m_blocksPerPage/4);

      return true;
    }

    break;
  case ACTION_PAGE_DOWN:
    {
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == m_channels - m_channelsPerPage || m_channels < m_channelsPerPage)
        { // already at the last page, so move to the last item.
          SetChannel(m_channels - m_channelOffset - 1);
        }
        else
        { // scroll down to the next page
          ChannelScroll(m_channelsPerPage);
        }
      }
      else
        ProgrammesScroll(m_blocksPerPage/4);

      return true;
    }

    break;

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
    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(GetSelectedItem());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetPointer())
    {
      Reset();
      CFileItemList *items = (CFileItemList *)message.GetPointer();

      /* Create Channel items */
      int iLastChannelNumber = -1;
      ItemsPtr itemsPointer;
      itemsPointer.start = 0;
      for (int i = 0; i < items->Size(); ++i)
      {
        const CEpgInfoTag* tag = items->Get(i)->GetEPGInfoTag();
        if (!tag || !tag->HasPVRChannel())
          continue;

        int iCurrentChannelNumber = tag->PVRChannelNumber();
        if (iCurrentChannelNumber != iLastChannelNumber)
        {
          CPVRChannelPtr channel = tag->ChannelTag();
          if (!channel)
            continue;

          if (i > 0)
          {
            itemsPointer.stop = i-1;
            m_epgItemsPtr.push_back(itemsPointer);
            itemsPointer.start = i;
          }
          iLastChannelNumber = iCurrentChannelNumber;
          CGUIListItemPtr item(new CFileItem(*channel));
          m_channelItems.push_back(item);
        }
      }
      if (items->Size() > 0)
      {
        itemsPointer.stop = items->Size()-1;
        m_epgItemsPtr.push_back(itemsPointer);
      }

      /* Create programme items */
      for (int i = 0; i < items->Size(); i++)
        m_programmeItems.push_back(items->Get(i));

      ClearGridIndex();
      m_gridIndex = (struct GridItemsPtr **) calloc(1,m_channelItems.size()*sizeof(struct GridItemsPtr*));
      if (m_gridIndex != NULL)
      {
        for (unsigned int i = 0; i < m_channelItems.size(); i++)
        {
          m_gridIndex[i] = (struct GridItemsPtr*) calloc(1,MAXBLOCKS*sizeof(struct GridItemsPtr));
        }
      }

      UpdateLayout(true); // true to refresh all items

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
      //SelectItem(message.GetParam1());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
    { // update our list contents
      for (unsigned int i = 0; i < m_channelItems.size(); ++i)
        m_channelItems[i]->SetInvalid();
      for (unsigned int i = 0; i < m_programmeItems.size(); ++i)
        m_programmeItems[i]->SetInvalid();
      for (unsigned int i = 0; i < m_rulerItems.size(); ++i)
        m_rulerItems[i]->SetInvalid();
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::UpdateItems()
{
  CDateTimeSpan blockDuration, gridDuration;

  /* check for invalid start and end time */
  if (m_gridStart >= m_gridEnd)
  {
    CLog::Log(LOGERROR, "CGUIEPGGridContainer - %s - invalid start and end time set", __FUNCTION__);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), GetParentID()); // message the window
    SendWindowMessage(msg);
    return;
  }

  gridDuration = m_gridEnd - m_gridStart;

  m_blocks = (gridDuration.GetDays()*24*60 + gridDuration.GetHours()*60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  if (m_blocks >= MAXBLOCKS)
    m_blocks = MAXBLOCKS;

  /* if less than one page, can't display grid */
  if (m_blocks < m_blocksPerPage)
  {
    CLog::Log(LOGERROR, "(%s) - Less than one page of data available.", __FUNCTION__);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), GetParentID()); // message the window
    SendWindowMessage(msg);
    return;
  }

  blockDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK, 0);

  long tick(XbmcThreads::SystemClockMillis());

  for (unsigned int row = 0; row < m_channelItems.size(); ++row)
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
          progIdx++;

        if (tag->EpgID() != iEpgId)
          break;

        if (m_gridEnd <= tag->StartAsUTC())
        {
          break;
        }
        else if (gridCursor >= tag->EndAsUTC())
        {
          progIdx++;
        }
        else if (gridCursor < tag->EndAsUTC())
        {
          m_gridIndex[row][block].item = item;
          break;
        }
        else
        {
          progIdx++;
        }
      }

      gridCursor += blockDuration;
    }

    /** FOR EACH BLOCK **********************************************************************/
    int itemSize = 1; // size of the programme in blocks
    int savedBlock = 0;

    for (int block = 0; block < m_blocks; block++)
    {
      if (m_gridIndex[row][block].item != m_gridIndex[row][block+1].item)
      {
        if (!m_gridIndex[row][block].item)
        {
          CEpgInfoTag broadcast;
          CFileItemPtr unknown(new CFileItem(broadcast));
          for (int i = block ; i > block - itemSize; i--)
          {
            m_gridIndex[row][i].item = unknown;
          }
        }

        CGUIListItemPtr item = m_gridIndex[row][block].item;
        CFileItem *fileItem = (CFileItem *)item.get();

        m_gridIndex[row][savedBlock].item->SetProperty("GenreType", fileItem->GetEPGInfoTag()->GenreType());
        if (m_orientation == VERTICAL)
        {
          m_gridIndex[row][savedBlock].width   = itemSize*m_blockSize;
          m_gridIndex[row][savedBlock].height  = m_channelHeight;
        }
        else
        {
          m_gridIndex[row][savedBlock].width   = m_channelWidth;
          m_gridIndex[row][savedBlock].height  = itemSize*m_blockSize;
        }

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

  CLog::Log(LOGDEBUG, "%s completed successfully in %u ms", __FUNCTION__, (unsigned int)(XbmcThreads::SystemClockMillis()-tick));

  m_channels = (int)m_epgItemsPtr.size();
  m_item = GetItem(m_channelCursor);
  if (m_item)
    SetBlock(GetBlock(m_item->item, m_channelCursor));

  SetInvalid();
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
  int offset = m_blockOffset + amount;

  if (offset > m_blocks - m_blocksPerPage)
  {
    offset = m_blocks - m_blocksPerPage;
  }

  if (offset < 0) offset = 0;

  ScrollToBlockOffset(offset);
}

bool CGUIEPGGridContainer::MoveChannel(bool direction, bool wrapAround)
{
  if (direction)
  {
    if (m_channelCursor > 0)
    {
      SetChannel(m_channelCursor - 1);
    }
    else if (m_channelCursor == 0 && m_channelOffset)
    {
      ScrollToChannelOffset(m_channelOffset - 1);
      SetChannel(0);
    }
    else if (wrapAround)
    {
      int offset = m_channels - m_channelsPerPage;

      if (offset < 0) offset = 0;

      SetChannel(m_channels - offset - 1);

      ScrollToChannelOffset(offset);
    }
    else
      return false;
  }
  else
  {
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
    else if (wrapAround)
    {
      SetChannel(0);
      ScrollToChannelOffset(0);
    }
    else
      return false;
  }
  return true;
}

bool CGUIEPGGridContainer::MoveProgrammes(bool direction)
{
  if (!m_gridIndex || !m_item)
    return false;

  if (direction)
  {
    if (m_channelCursor + m_channelOffset < 0 || m_blockOffset < 0)
      return false;

    if (m_item->item != m_gridIndex[m_channelCursor + m_channelOffset][m_blockOffset].item)
    {
      // this is not first item on page
      m_item = GetPrevItem(m_channelCursor);
      SetBlock(GetBlock(m_item->item, m_channelCursor));
    }
    else if (m_blockCursor <= 0 && m_blockOffset)
    {
      // we're at the left edge and offset
      int itemSize = GetItemSize(m_item);
      int block = GetRealBlock(m_item->item, m_channelCursor);

      if (block < m_blockOffset) /* current item begins before current offset, keep selected */
      {
        if (itemSize > m_blocksPerPage) /* current item is longer than one page, scroll one page left */
        {
          m_blockOffset < m_blocksPerPage ? block = 0 : block = m_blockOffset - m_blocksPerPage; // number blocks left < m_blocksPerPAge
          ScrollToBlockOffset(block);
          SetBlock(0);
        }
        else /* current item is shorter than one page, scroll left to start of item */
        {
          ScrollToBlockOffset(block); // -1?
          SetBlock(0); // align cursor to left edge
        }
      }
      else /* current item starts on this page's edge, select the previous item */
      {
        m_item = GetPrevItem(m_channelCursor);
        itemSize = GetItemSize(m_item);

        if (itemSize > m_blocksPerPage) // previous item is longer than one page, scroll left to last page of item */
        {
          ScrollToBlockOffset(m_blockOffset - m_blocksPerPage); // left one whole page
          //SetBlock(m_blocksPerPage -1 ); // helps navigation by setting cursor to far right edge
          SetBlock(0); // align cursor to left edge
        }
        else /* previous item is shorter than one page, scroll left to start of item */
        {
          ScrollToBlockOffset(m_blockOffset - itemSize);
          SetBlock(0); //should be zero
        }
      }
    }
    else
      return false;
  }
  else
  {
    if (m_item->item != m_gridIndex[m_channelCursor + m_channelOffset][m_blocksPerPage + m_blockOffset - 1].item)
    {
      // this is not last item on page
      m_item = GetNextItem(m_channelCursor);
      SetBlock(GetBlock(m_item->item, m_channelCursor));
    }
    else if ((m_blockOffset != m_blocks - m_blocksPerPage) && m_blocks > m_blocksPerPage)
    {
      // at right edge, more than one page and not at maximum offset
      int itemSize = GetItemSize(m_item);
      int block = GetRealBlock(m_item->item, m_channelCursor);

      if (itemSize > m_blocksPerPage - m_blockCursor) // current item extends into next page, keep selected
      {
        if (itemSize > m_blocksPerPage) // current item is longer than one page, scroll one page right
        {
          if (m_blockOffset && m_blockOffset + m_blocksPerPage > m_blocks)
            block = m_blocks - m_blocksPerPage;
          else
            block = m_blockOffset + m_blocksPerPage;

          ScrollToBlockOffset(block);

          SetBlock(0);
        }
        else // current item is shorter than one page, scroll so end of item sits on end of grid
        {
          ScrollToBlockOffset(block + itemSize - m_blocksPerPage);
          SetBlock(GetBlock(m_item->item, m_channelCursor)); /// change to middle block of item?
        }
      }
      else // current item finishes on this page's edge, select the next item
      {
        m_item = GetNextItem(m_channelCursor);
        itemSize = GetItemSize(m_item);

        if (itemSize > m_blocksPerPage) // next item is longer than one page, scroll to first page of this item
        {
          ScrollToBlockOffset(m_blockOffset + m_blocksPerPage);
          SetBlock(0);
        }
        else // next item is shorter than one page, scroll so end of item sits on end of grid
        {
          ScrollToBlockOffset(m_blockOffset + itemSize);
          SetBlock(m_blocksPerPage - itemSize); /// change to middle block of item?
        }
      }
    }
    else
      return false;
  }
  return true;
}

void CGUIEPGGridContainer::OnUp()
{
  bool wrapAround = m_actionUp.GetNavigation() == GetID() || !m_actionUp.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL)
  {
    if (!MoveChannel(true, wrapAround))
      CGUIControl::OnUp();
  }
  else
  {
    if (!MoveProgrammes(true))
      CGUIControl::OnUp();
  }
}

void CGUIEPGGridContainer::OnDown()
{
  bool wrapAround = m_actionDown.GetNavigation() == GetID() || !m_actionDown.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL)
  {
    if (!MoveChannel(false, wrapAround))
      CGUIControl::OnDown();
  }
  else
  {
    if (!MoveProgrammes(false))
      CGUIControl::OnDown();
  }
}

void CGUIEPGGridContainer::OnLeft()
{
  bool wrapAround = m_actionLeft.GetNavigation() == GetID() || !m_actionLeft.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL)
  {
    if (!MoveProgrammes(true))
      CGUIControl::OnLeft();
  }
  else
  {
    if (!MoveChannel(true, wrapAround))
      CGUIControl::OnLeft();
  }
}

void CGUIEPGGridContainer::OnRight()
{
  bool wrapAround = m_actionRight.GetNavigation() == GetID() || !m_actionRight.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL)
  {
    if (!MoveProgrammes(false))
      CGUIControl::OnRight();
  }
  else
  {
    if (!MoveChannel(false, wrapAround))
      CGUIControl::OnRight();
  }
}

void CGUIEPGGridContainer::SetChannel(const CStdString &channel)
{
  int iChannelIndex(-1);
  for (unsigned int iIndex = 0; iIndex < m_channelItems.size(); iIndex++)
  {
    CStdString strPath = m_channelItems[iIndex]->GetProperty("path").asString(StringUtils::EmptyString);
    if (strPath == channel)
    {
      iChannelIndex = iIndex;
      break;
    }
  }

  if (iChannelIndex >= 0)
    ScrollToChannelOffset(iChannelIndex);
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

  if (iChannelIndex >= 0)
    ScrollToChannelOffset(iChannelIndex);
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  if (m_blockCursor + m_blockOffset == 0 || m_blockOffset + m_blockCursor + GetItemSize(m_item) == m_blocks)
  {
    m_item          = GetItem(channel);
    if (m_item)
    {
      SetBlock(GetBlock(m_item->item, channel));
      m_channelCursor = channel;
    }
    return;
  }

  /* basic checks failed, need to correctly identify nearest item */
  m_item          = GetClosestItem(channel);
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
  m_item        = GetItem(m_channelCursor);
}

CGUIListItemLayout *CGUIEPGGridContainer::GetFocusedLayout() const
{
  CGUIListItemPtr item = GetListItem(0);

  if (item.get()) return item->GetFocusedLayout();

  return NULL;
}

bool CGUIEPGGridContainer::SelectItemFromPoint(const CPoint &point)
{
  /* point has already had origin set to m_posX, m_posY */
  if (!m_focusedProgrammeLayout || !m_programmeLayout)
    return false;

  int channel = (int)(point.y / m_channelHeight);
  int block   = (int)(point.x / m_blockSize);

  if (channel > m_channelsPerPage) channel = m_channelsPerPage - 1;
  if (channel >= m_channels) channel = m_channels - 1;
  if (channel < 0) channel = 0;
  if (block > m_blocksPerPage) block = m_blocksPerPage - 1;
  if (block < 0) block = 0;

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
  default:
    return EVENT_RESULT_UNHANDLED;
  }
}

bool CGUIEPGGridContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_gridPosX, m_posY + m_rulerHeight));
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

int CGUIEPGGridContainer::GetSelectedItem() const
{
  if (!m_gridIndex ||
      !m_epgItemsPtr.size() ||
      m_channelCursor + m_channelOffset >= (int)m_channelItems.size() ||
      m_blockCursor + m_blockOffset >= (int)m_programmeItems.size())
    return 0;

  CGUIListItemPtr currentItem = m_gridIndex[m_channelCursor + m_channelOffset][m_blockCursor + m_blockOffset].item;
  if (!currentItem)
    return 0;

  for (int i = 0; i < (int)m_programmeItems.size(); i++)
  {
    if (currentItem == m_programmeItems[i])
      return i;
  }
  return 0;
}

CGUIListItemPtr CGUIEPGGridContainer::GetListItem(int offset) const
{
  if (!m_epgItemsPtr.size())
    return CGUIListItemPtr();

  return m_item->item;
}

GridItemsPtr *CGUIEPGGridContainer::GetClosestItem(const int &channel)
{
  GridItemsPtr *closest = GetItem(channel);

  if(!closest)
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

  return (int) ((m_orientation == VERTICAL ? item->width : item->height) / m_blockSize);
}

int CGUIEPGGridContainer::GetBlock(const CGUIListItemPtr &item, const int &channel)
{
  if (!item)
    return 0;

  return GetRealBlock(item, channel) - m_blockOffset;
}

int CGUIEPGGridContainer::GetRealBlock(const CGUIListItemPtr &item, const int &channel)
{
  int block = 0;

  while (m_gridIndex[channel + m_channelOffset][block].item != item && block < m_blocks)
    block++;

  return block;
}

GridItemsPtr *CGUIEPGGridContainer::GetNextItem(const int &channel)
{
  int i = m_blockCursor;

  while (m_gridIndex[channel + m_channelOffset][i + m_blockOffset].item == m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset].item && i < m_blocksPerPage)
    i++;

  return &m_gridIndex[channel + m_channelOffset][i + m_blockOffset];
}

GridItemsPtr *CGUIEPGGridContainer::GetPrevItem(const int &channel)
{
  int i = m_blockCursor;

  while (m_gridIndex[channel + m_channelOffset][i + m_blockOffset].item == m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset].item && i > 0)
    i--;

  return &m_gridIndex[channel + m_channelOffset][i + m_blockOffset];

//  return &m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset - 1];
}

GridItemsPtr *CGUIEPGGridContainer::GetItem(const int &channel)
{
  if ( (channel >= 0) && (channel < m_channels) )
    return &m_gridIndex[channel + m_channelOffset][m_blockCursor + m_blockOffset];
  else
    return NULL;
}

void CGUIEPGGridContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
  {
    SetInvalid();
    /*m_lastItem.reset();
    m_lastChannel.reset();*/
  }

  CGUIControl::SetFocus(bOnOff);
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

  if (m_channelOffset > m_channels - m_channelsPerPage)
  {
    m_channelOffset = m_channels - m_channelsPerPage;
    m_channelScrollOffset = m_channelOffset * m_channelHeight;
  }

  if (m_channelOffset < 0)
  {
    m_channelOffset = 0;
    m_channelScrollOffset = 0;
  }

  if (m_blockOffset > m_blocks - m_blocksPerPage)
  {
    m_blockOffset = m_blocks - m_blocksPerPage;
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
  }

  if (m_blockOffset < 0)
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

void CGUIEPGGridContainer::UpdateLayout(bool updateAllItems)
{
  // if container is invalid, either new data has arrived, or m_blockSize has changed
  //  need to run UpdateItems rather than CalculateLayout?
  if (updateAllItems)
  { // free memory of items
    for (iItems it = m_channelItems.begin(); it != m_channelItems.end(); it++)
      (*it)->FreeMemory();
    for (iItems it = m_rulerItems.begin(); it != m_rulerItems.end(); it++)
      (*it)->FreeMemory();
    for (iItems it = m_programmeItems.begin(); it != m_programmeItems.end(); it++)
      (*it)->FreeMemory();
  }

  // and recalculate the layout
  CalculateLayout();
}

CStdString CGUIEPGGridContainer::GetDescription() const
{
  CStdString strLabel;
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
  if (m_gridIndex)
  {
    for (unsigned int i = 0; i < m_channelItems.size(); i++)
    {
      for (int block = 0; block < m_blocks; block++)
      {
        if (m_gridIndex[i][block].item)
          m_gridIndex[i][block].item.get()->ClearProperties();
      }
      free(m_gridIndex[i]);
    }
    free(m_gridIndex);
  }
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
  m_gridIndex   = NULL;
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

void CGUIEPGGridContainer::SetStartEnd(CDateTime start, CDateTime end)
{
  m_gridStart = CDateTime(start.GetYear(), start.GetMonth(), start.GetDay(), start.GetHour(), start.GetMinute() >= 30 ? 30 : 0, 0);
  m_gridEnd = CDateTime(end.GetYear(), end.GetMonth(), end.GetDay(), end.GetHour(), end.GetMinute() >= 30 ? 30 : 0, 0);

  CLog::Log(LOGDEBUG, "CGUIEPGGridContainer - %s - start=%s end=%s",
      __FUNCTION__, m_gridStart.GetAsLocalizedDateTime(false, true).c_str(), m_gridEnd.GetAsLocalizedDateTime(false, true).c_str());
}

void CGUIEPGGridContainer::CalculateLayout()
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

  m_channelHeight       = m_channelLayout->Size(VERTICAL);
  m_channelWidth        = m_channelLayout->Size(HORIZONTAL);
  if (m_orientation == VERTICAL)
  {
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
    m_ProgrammesPerPage = (int)(m_gridWidth / m_blockSize) + 1;
  }
  else
  {
    m_rulerWidth        = m_rulerLayout->Size(HORIZONTAL);
    m_gridPosX          = m_posX + m_rulerWidth;
    m_gridPosY          = m_posY + m_channelHeight;
    m_gridWidth         = m_width - m_rulerWidth;
    m_gridHeight        = m_height - m_channelHeight;
    m_blockSize         = m_gridHeight / m_blocksPerPage;
    m_rulerHeight       = m_rulerUnit * m_blockSize;
    m_channelPosX       = m_posX + m_rulerWidth;
    m_channelPosY       = m_posY;
    m_rulerPosX         = m_posX;
    m_rulerPosY         = m_posY + m_channelHeight;
    m_channelsPerPage   = (int)(m_gridWidth / m_channelWidth);
    m_ProgrammesPerPage = (int)(m_gridHeight / m_blockSize) + 1;
  }

  // ensure that the scroll offsets are a multiple of our sizes
  m_channelScrollOffset   = m_channelOffset * m_programmeLayout->Size(m_orientation);
  m_programmeScrollOffset = m_blockOffset * m_blockSize;
}

void CGUIEPGGridContainer::UpdateScrollOffset()
{
  m_channelScrollOffset += m_channelScrollSpeed * (m_renderTime - m_channelScrollLastTime);
  if ((m_channelScrollSpeed < 0 && m_channelScrollOffset < m_channelOffset * m_programmeLayout->Size(m_orientation)) ||
      (m_channelScrollSpeed > 0 && m_channelScrollOffset > m_channelOffset * m_programmeLayout->Size(m_orientation)))
  {
    m_channelScrollOffset = m_channelOffset * m_programmeLayout->Size(m_orientation);
    m_channelScrollSpeed = 0;
  }
  m_channelScrollLastTime = m_renderTime;

  m_programmeScrollOffset += m_programmeScrollSpeed * (m_renderTime - m_programmeScrollLastTime);
  if ((m_programmeScrollSpeed < 0 && m_programmeScrollOffset < m_blockOffset * m_blockSize) ||
      (m_programmeScrollSpeed > 0 && m_programmeScrollOffset > m_blockOffset * m_blockSize))
  {
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
    m_programmeScrollSpeed = 0;
  }
  m_programmeScrollLastTime = m_renderTime;
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
  if (!m_channelLayout && m_channelLayouts.size())
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
  if (!m_focusedChannelLayout && m_focusedChannelLayouts.size())
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
  if (!m_programmeLayout && m_programmeLayouts.size())
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
  if (!m_focusedProgrammeLayout && m_focusedProgrammeLayouts.size())
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
  if (!m_rulerLayout && m_rulerLayouts.size())
    m_rulerLayout = &m_rulerLayouts[0];  // failsafe
}

int CGUIEPGGridContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIEPGGridContainer::SetRenderOffset(const CPoint &offset)
{
  m_renderOffset = offset;
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

void CGUIEPGGridContainer::FreeProgrammeMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    for (unsigned int i = 0; i < m_epgItemsPtr.size(); i++)
    {
      unsigned long progIdx = m_epgItemsPtr[i].start;
      unsigned long lastIdx = m_epgItemsPtr[i].stop;

      for (unsigned int j = progIdx; j < keepStart+progIdx && j < lastIdx; ++j)
        m_programmeItems[j]->FreeMemory();
      for (unsigned int j = keepEnd+progIdx + 1; j < lastIdx; ++j)
        m_programmeItems[j]->FreeMemory();
    }
  }
  else
  { // wrapping
    for (unsigned int i = 0; i < m_epgItemsPtr.size(); i++)
    {
      unsigned long progIdx = m_epgItemsPtr[i].start;
      unsigned long lastIdx = m_epgItemsPtr[i].stop;

      for (unsigned int j = keepEnd+progIdx + 1; j < keepStart+progIdx && j < lastIdx; ++j)
        m_programmeItems[j]->FreeMemory();
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

void CGUIEPGGridContainer::GetRulerCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_programmeScrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheRulerItems;
  }
  else if (m_programmeScrollSpeed < 0)
  {
    cacheBefore = m_cacheRulerItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheRulerItems / 2;
    cacheAfter = m_cacheRulerItems / 2;
  }
}
