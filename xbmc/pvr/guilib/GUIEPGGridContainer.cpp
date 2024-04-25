/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIEPGGridContainer.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "guilib/DirtyRegion.h"
#include "guilib/GUIAction.h"
#include "guilib/GUIMessage.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/GUIEPGGridContainerModel.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <tinyxml.h>

using namespace KODI;
using namespace PVR;

#define BLOCKJUMP    4 // how many blocks are jumped with each analogue scroll action
static const int BLOCK_SCROLL_OFFSET = 60 / CGUIEPGGridContainerModel::MINSPERBLOCK; // how many blocks are jumped if we are at left/right edge of grid

CGUIEPGGridContainer::CGUIEPGGridContainer(int parentID,
                                           int controlID,
                                           float posX,
                                           float posY,
                                           float width,
                                           float height,
                                           ORIENTATION orientation,
                                           int scrollTime,
                                           int preloadItems,
                                           int timeBlocks,
                                           int rulerUnit,
                                           const CTextureInfo& progressIndicatorTexture)
  : IGUIContainer(parentID, controlID, posX, posY, width, height),
    m_orientation(orientation),
    m_rulerUnit(rulerUnit),
    m_blocksPerPage(timeBlocks),
    m_cacheChannelItems(preloadItems),
    m_cacheProgrammeItems(preloadItems),
    m_cacheRulerItems(preloadItems),
    m_guiProgressIndicatorTexture(
        CGUITexture::CreateTexture(posX, posY, width, height, progressIndicatorTexture)),
    m_scrollTime(scrollTime ? scrollTime : 1),
    m_gridModel(new CGUIEPGGridContainerModel)
{
  ControlType = GUICONTAINER_EPGGRID;
}

CGUIEPGGridContainer::CGUIEPGGridContainer(const CGUIEPGGridContainer& other)
  : IGUIContainer(other),
    m_renderOffset(other.m_renderOffset),
    m_orientation(other.m_orientation),
    m_channelLayouts(other.m_channelLayouts),
    m_focusedChannelLayouts(other.m_focusedChannelLayouts),
    m_focusedProgrammeLayouts(other.m_focusedProgrammeLayouts),
    m_programmeLayouts(other.m_programmeLayouts),
    m_rulerLayouts(other.m_rulerLayouts),
    m_rulerDateLayouts(other.m_rulerDateLayouts),
    m_channelLayout(other.m_channelLayout),
    m_focusedChannelLayout(other.m_focusedChannelLayout),
    m_programmeLayout(other.m_programmeLayout),
    m_focusedProgrammeLayout(other.m_focusedProgrammeLayout),
    m_rulerLayout(other.m_rulerLayout),
    m_rulerDateLayout(other.m_rulerDateLayout),
    m_pageControl(other.m_pageControl),
    m_rulerUnit(other.m_rulerUnit),
    m_channelsPerPage(other.m_channelsPerPage),
    m_programmesPerPage(other.m_programmesPerPage),
    m_channelCursor(other.m_channelCursor),
    m_channelOffset(other.m_channelOffset),
    m_blocksPerPage(other.m_blocksPerPage),
    m_blockCursor(other.m_blockCursor),
    m_blockOffset(other.m_blockOffset),
    m_blockTravelAxis(other.m_blockTravelAxis),
    m_cacheChannelItems(other.m_cacheChannelItems),
    m_cacheProgrammeItems(other.m_cacheProgrammeItems),
    m_cacheRulerItems(other.m_cacheRulerItems),
    m_rulerDateHeight(other.m_rulerDateHeight),
    m_rulerDateWidth(other.m_rulerDateWidth),
    m_rulerPosX(other.m_rulerPosX),
    m_rulerPosY(other.m_rulerPosY),
    m_rulerHeight(other.m_rulerHeight),
    m_rulerWidth(other.m_rulerWidth),
    m_channelPosX(other.m_channelPosX),
    m_channelPosY(other.m_channelPosY),
    m_channelHeight(other.m_channelHeight),
    m_channelWidth(other.m_channelWidth),
    m_gridPosX(other.m_gridPosX),
    m_gridPosY(other.m_gridPosY),
    m_gridWidth(other.m_gridWidth),
    m_gridHeight(other.m_gridHeight),
    m_blockSize(other.m_blockSize),
    m_analogScrollCount(other.m_analogScrollCount),
    m_guiProgressIndicatorTexture(other.m_guiProgressIndicatorTexture->Clone()),
    m_lastItem(other.m_lastItem),
    m_lastChannel(other.m_lastChannel),
    m_scrollTime(other.m_scrollTime),
    m_programmeScrollLastTime(other.m_programmeScrollLastTime),
    m_programmeScrollSpeed(other.m_programmeScrollSpeed),
    m_programmeScrollOffset(other.m_programmeScrollOffset),
    m_channelScrollLastTime(other.m_channelScrollLastTime),
    m_channelScrollSpeed(other.m_channelScrollSpeed),
    m_channelScrollOffset(other.m_channelScrollOffset),
    m_gridModel(new CGUIEPGGridContainerModel(*other.m_gridModel)),
    m_updatedGridModel(other.m_updatedGridModel
                           ? new CGUIEPGGridContainerModel(*other.m_updatedGridModel)
                           : nullptr),
    m_itemStartBlock(other.m_itemStartBlock)
{
}

bool CGUIEPGGridContainer::HasData() const
{
  return m_gridModel && m_gridModel->HasChannelItems();
}

void CGUIEPGGridContainer::AllocResources()
{
  IGUIContainer::AllocResources();
  m_guiProgressIndicatorTexture->AllocResources();
}

void CGUIEPGGridContainer::FreeResources(bool immediately)
{
  m_guiProgressIndicatorTexture->FreeResources(immediately);
  IGUIContainer::FreeResources(immediately);
}

void CGUIEPGGridContainer::SetPageControl(int id)
{
  m_pageControl = id;
}

void CGUIEPGGridContainer::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  ValidateOffset();

  if (m_bInvalidated)
  {
    UpdateLayout();

    if (m_pageControl)
    {
      int iItemsPerPage;
      int iTotalItems;

      if (m_orientation == VERTICAL)
      {
        iItemsPerPage = m_channelsPerPage;
        iTotalItems = m_gridModel->ChannelItemsSize();
      }
      else
      {
        iItemsPerPage = m_blocksPerPage;
        iTotalItems = m_gridModel->GridItemsSize();
      }

      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, iItemsPerPage, iTotalItems);
      SendWindowMessage(msg);
    }
  }

  UpdateScrollOffset(currentTime);
  ProcessChannels(currentTime, dirtyregions);
  ProcessRulerDate(currentTime, dirtyregions);
  ProcessRuler(currentTime, dirtyregions);
  ProcessProgrammeGrid(currentTime, dirtyregions);
  ProcessProgressIndicator(currentTime, dirtyregions);

  if (m_pageControl)
  {
    int iItem =
        (m_orientation == VERTICAL)
            ? MathUtils::round_int(static_cast<double>(m_channelScrollOffset / m_channelHeight))
            : MathUtils::round_int(
                  static_cast<double>(m_programmeScrollOffset / (m_gridHeight / m_blocksPerPage)));

    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, iItem);
    SendWindowMessage(msg);
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIEPGGridContainer::Render()
{
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
  {
    RenderProgressIndicator();
    RenderProgrammeGrid();
    RenderRuler();
    RenderRulerDate();
    RenderChannels();
  }
  else
  {
    RenderChannels();
    RenderRulerDate();
    RenderRuler();
    RenderProgrammeGrid();
    RenderProgressIndicator();
  }

  CGUIControl::Render();
}

void CGUIEPGGridContainer::ProcessChannels(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  HandleChannels(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderChannels()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleChannels(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessRulerDate(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  HandleRulerDate(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderRulerDate()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleRulerDate(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessRuler(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  HandleRuler(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderRuler()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleRuler(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessProgrammeGrid(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  HandleProgrammeGrid(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderProgrammeGrid()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleProgrammeGrid(true, dummyTime, dummyRegions);
}

float CGUIEPGGridContainer::GetCurrentTimePositionOnPage() const
{
  if (!m_gridModel->GetGridStart().IsValid())
    return -1.0f;

  const CDateTimeSpan startDelta(CDateTime::GetUTCDateTime() - m_gridModel->GetGridStart());
  const float fPos = (startDelta.GetSecondsTotal() * m_blockSize) /
                         (CGUIEPGGridContainerModel::MINSPERBLOCK * 60) -
                     GetProgrammeScrollOffsetPos();
  return std::min(fPos, m_orientation == VERTICAL ? m_gridWidth : m_gridHeight);
}

float CGUIEPGGridContainer::GetProgressIndicatorWidth() const
{
  return (m_orientation == VERTICAL) ? GetCurrentTimePositionOnPage() : m_rulerWidth + m_gridWidth;
}

float CGUIEPGGridContainer::GetProgressIndicatorHeight() const
{
  return (m_orientation == VERTICAL) ? m_rulerHeight + m_gridHeight : GetCurrentTimePositionOnPage();
}

void CGUIEPGGridContainer::ProcessProgressIndicator(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  float width = GetProgressIndicatorWidth();
  float height = GetProgressIndicatorHeight();

  if (width > 0 && height > 0)
  {
    m_guiProgressIndicatorTexture->SetVisible(true);
    m_guiProgressIndicatorTexture->SetPosition(m_rulerPosX + m_renderOffset.x,
                                               m_rulerPosY + m_renderOffset.y);
    m_guiProgressIndicatorTexture->SetWidth(width);
    m_guiProgressIndicatorTexture->SetHeight(height);
  }
  else
  {
    m_guiProgressIndicatorTexture->SetVisible(false);
  }

  m_guiProgressIndicatorTexture->Process(currentTime);
}

void CGUIEPGGridContainer::RenderProgressIndicator()
{
  if (CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_rulerPosX, m_rulerPosY, GetProgressIndicatorWidth(), GetProgressIndicatorHeight()))
  {
    m_guiProgressIndicatorTexture->SetDiffuseColor(m_diffuseColor);
    m_guiProgressIndicatorTexture->Render(0, m_guiProgressIndicatorTextureDepth);
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  }
}

void CGUIEPGGridContainer::ProcessItem(float posX, float posY, const CFileItemPtr& item, CFileItemPtr& lastitem,
  bool focused, CGUIListItemLayout* normallayout, CGUIListItemLayout* focusedlayout,
  unsigned int currentTime, CDirtyRegionList& dirtyregions, float resize /* = -1.0f */)
{
  if (!normallayout || !focusedlayout)
    return;

  // set the origin
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();

  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      item->SetFocusedLayout(std::make_unique<CGUIListItemLayout>(*focusedlayout, this));
    }

    if (resize != -1.0f)
    {
      if (m_orientation == VERTICAL)
        item->GetFocusedLayout()->SetWidth(resize);
      else
        item->GetFocusedLayout()->SetHeight(resize);
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

    item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    lastitem = item;
  }
  else
  {
    if (!item->GetLayout())
    {
      item->SetLayout(std::make_unique<CGUIListItemLayout>(*normallayout, this));
    }

    if (resize != -1.0f)
    {
      if (m_orientation == VERTICAL)
        item->GetLayout()->SetWidth(resize);
      else
        item->GetLayout()->SetHeight(resize);
    }

    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);

    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    else
      item->GetLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
  }
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIListItem* item, bool focused)
{
  // set the origin
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(posX, posY);

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
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

bool CGUIEPGGridContainer::OnAction(const CAction& action)
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
      ScrollToBlockOffset(m_blockOffset + (12 * 60  / CGUIEPGGridContainerModel::MINSPERBLOCK));
      SetBlock(m_blockCursor);
      return true;

    case ACTION_PREV_ITEM:
      // skip -12h
      ScrollToBlockOffset(m_blockOffset - (12 * 60 / CGUIEPGGridContainerModel::MINSPERBLOCK));
      SetBlock(m_blockCursor);
      return true;

    case REMOTE_0:
      GoToNow();
      return true;

    case ACTION_PAGE_UP:
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == 0)
        {
          // already on the first page, so move to the first item
          SetChannel(0);
        }
        else
        {
          // scroll up to the previous page
          ChannelScroll(-m_channelsPerPage);
        }
      }
      else
      {
        if (m_blockOffset == 0)
        {
          // already on the first page, so move to the first item
          SetBlock(0);
        }
        else
        {
          // scroll up to the previous page
          ProgrammesScroll(-m_blocksPerPage);
        }
      }
      return true;

    case ACTION_PAGE_DOWN:
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == m_gridModel->ChannelItemsSize() - m_channelsPerPage ||
            m_gridModel->ChannelItemsSize() < m_channelsPerPage)
        {
          // already at the last page, so move to the last item.
          SetChannel(m_gridModel->GetLastChannel() - m_channelOffset);
        }
        else
        {
          // scroll down to the next page
          ChannelScroll(m_channelsPerPage);
        }
      }
      else
      {
        if (m_blockOffset == m_gridModel->GridItemsSize() - m_blocksPerPage ||
            m_gridModel->GridItemsSize() < m_blocksPerPage)
        {
          // already at the last page, so move to the last item.
          SetBlock(m_gridModel->GetLastBlock() - m_blockOffset);
        }
        else
        {
          // scroll down to the next page
          ProgrammesScroll(m_blocksPerPage);
        }
      }

      return true;

    // smooth scrolling (for analog controls)
    case ACTION_TELETEXT_RED:
    case ACTION_TELETEXT_GREEN:
    case ACTION_SCROLL_UP: // left horizontal scrolling
      if (m_orientation == VERTICAL)
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_RED ? m_blocksPerPage / 2 : m_blocksPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4f)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset > 0 && m_blockCursor <= m_blocksPerPage / 2)
            ProgrammesScroll(-blocksToJump);
          else if (m_blockCursor > blocksToJump)
            SetBlock(m_blockCursor - blocksToJump);
        }
        return handled;
      }
      else
      {
        int channelsToJump = action.GetID() == ACTION_TELETEXT_RED ? m_channelsPerPage / 2 : m_channelsPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4f)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_channelOffset > 0 && m_channelCursor <= m_channelsPerPage / 2)
            ChannelScroll(-channelsToJump);
          else if (m_channelCursor > channelsToJump)
            SetChannel(m_channelCursor - channelsToJump);
        }
        return handled;
      }
      break;

    case ACTION_TELETEXT_BLUE:
    case ACTION_TELETEXT_YELLOW:
    case ACTION_SCROLL_DOWN: // right horizontal scrolling
      if (m_orientation == VERTICAL)
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_BLUE ? m_blocksPerPage / 2 : m_blocksPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4f)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset + m_blocksPerPage < m_gridModel->GridItemsSize() &&
              m_blockCursor >= m_blocksPerPage / 2)
            ProgrammesScroll(blocksToJump);
          else if (m_blockCursor < m_blocksPerPage - blocksToJump &&
                   m_blockOffset + m_blockCursor < m_gridModel->GridItemsSize() - blocksToJump)
            SetBlock(m_blockCursor + blocksToJump);
        }
        return handled;
      }
      else
      {
        int channelsToJump = action.GetID() == ACTION_TELETEXT_BLUE ? m_channelsPerPage / 2 : m_channelsPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4f)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_channelOffset + m_channelsPerPage < m_gridModel->ChannelItemsSize() && m_channelCursor >= m_channelsPerPage / 2)
            ChannelScroll(channelsToJump);
          else if (m_channelCursor < m_channelsPerPage - channelsToJump && m_channelOffset + m_channelCursor < m_gridModel->ChannelItemsSize() - channelsToJump)
            SetChannel(m_channelCursor + channelsToJump);
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
      case GUI_MSG_PAGE_CHANGE:
        if (message.GetSenderId() == m_pageControl && IsVisible())
        {
          if (m_orientation == VERTICAL)
          {
            ScrollToChannelOffset(message.GetParam1());
            SetChannel(m_channelCursor);
          }
          else
          {
            ScrollToBlockOffset(message.GetParam1());
            SetBlock(m_blockCursor);
          }
          return true;
        }
        break;

      case GUI_MSG_LABEL_BIND:
        UpdateItems();
        return true;

      case GUI_MSG_REFRESH_LIST:
        // update our list contents
        m_gridModel->SetInvalid();
        break;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::UpdateItems()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!m_updatedGridModel)
    return;

  // Save currently selected epg tag and grid coordinates. Selection shall be restored after update.
  std::shared_ptr<CPVREpgInfoTag> prevSelectedEpgTag;
  if (HasData())
    prevSelectedEpgTag =
        m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset)
            ->GetEPGInfoTag();

  const int oldChannelIndex = m_channelOffset + m_channelCursor;
  const int oldBlockIndex = m_blockOffset + m_blockCursor;
  const CDateTime oldGridStart(m_gridModel->GetGridStart());
  int eventOffset = oldBlockIndex;
  int newChannelIndex = oldChannelIndex;
  int newBlockIndex = oldBlockIndex;
  int channelUid = -1;
  unsigned int broadcastUid = 0;

  if (prevSelectedEpgTag)
  {
    // get the block offset relative to the first block of the selected event
    eventOffset =
        oldBlockIndex - m_gridModel->GetGridItemStartBlock(oldChannelIndex, oldBlockIndex);

    if (!prevSelectedEpgTag->IsGapTag()) // "normal" tag selected
    {
      if (oldGridStart >= prevSelectedEpgTag->StartAsUTC())
      {
        // start of previously selected event is before grid start
        newBlockIndex = eventOffset;
      }
      else
      {
        newBlockIndex = m_gridModel->GetFirstEventBlock(prevSelectedEpgTag) + eventOffset;
      }

      channelUid = prevSelectedEpgTag->UniqueChannelID();
      broadcastUid = prevSelectedEpgTag->UniqueBroadcastID();
    }
    else // "gap" tag selected
    {
      channelUid = prevSelectedEpgTag->UniqueChannelID();

      // As gap tags do not have a unique broadcast id, we will look for the real tag preceding
      // the gap tag and add the respective offset to restore the gap tag selection, assuming that
      // the real tag is still the predecessor of the gap tag after the grid model update.

      const std::shared_ptr<CFileItem> prevItem = GetPrevItem().first;
      if (prevItem)
      {
        const std::shared_ptr<const CPVREpgInfoTag> tag = prevItem->GetEPGInfoTag();
        if (tag && !tag->IsGapTag())
        {
          if (oldGridStart >= tag->StartAsUTC())
          {
            // start of previously selected event is before grid start
            newBlockIndex = eventOffset;
          }
          else
          {
            newBlockIndex = m_gridModel->GetFirstEventBlock(tag);
            eventOffset += m_gridModel->GetFirstEventBlock(prevSelectedEpgTag) - newBlockIndex;
          }

          broadcastUid = tag->UniqueBroadcastID();
        }
      }
    }
  }

  m_lastItem = nullptr;
  m_lastChannel = nullptr;

  // always use asynchronously precalculated grid data.
  m_gridModel = std::move(m_updatedGridModel);

  if (prevSelectedEpgTag)
  {
    if (oldGridStart != m_gridModel->GetGridStart())
    {
      // grid start changed. block offset for selected event might have changed.
      newBlockIndex += m_gridModel->GetBlock(oldGridStart);
      if (newBlockIndex < 0 || newBlockIndex > m_gridModel->GetLastBlock())
      {
        // previous selection is no longer in grid.
        SetInvalid();
        m_bEnableChannelScrolling = false;
        GoToChannel(newChannelIndex);
        m_bEnableProgrammeScrolling = false;
        GoToNow();
        return;
      }
    }

    if (newChannelIndex >= m_gridModel->ChannelItemsSize() ||
        newBlockIndex >= m_gridModel->GridItemsSize() ||
        m_gridModel->GetGridItem(newChannelIndex, newBlockIndex)->GetEPGInfoTag() !=
            prevSelectedEpgTag)
    {
      int iChannelIndex = CGUIEPGGridContainerModel::INVALID_INDEX;
      int iBlockIndex = CGUIEPGGridContainerModel::INVALID_INDEX;
      m_gridModel->FindChannelAndBlockIndex(channelUid, broadcastUid, eventOffset, iChannelIndex, iBlockIndex);

      if (iBlockIndex != CGUIEPGGridContainerModel::INVALID_INDEX)
      {
        newBlockIndex = iBlockIndex;
      }
      else if (newBlockIndex > m_gridModel->GetLastBlock())
      {
        // default to now
        newBlockIndex = m_gridModel->GetNowBlock();

        if (newBlockIndex > m_gridModel->GetLastBlock())
        {
          // last block is in the past. default to last block
          newBlockIndex = m_gridModel->GetLastBlock();
        }
      }

      if (iChannelIndex != CGUIEPGGridContainerModel::INVALID_INDEX)
      {
        newChannelIndex = iChannelIndex;
      }
      else if (newChannelIndex >= m_gridModel->ChannelItemsSize() ||
               (m_gridModel->GetGridItem(newChannelIndex, newBlockIndex)->GetEPGInfoTag()->UniqueChannelID() != prevSelectedEpgTag->UniqueChannelID() &&
                m_gridModel->GetGridItem(newChannelIndex, newBlockIndex)->GetEPGInfoTag()->ClientID() != prevSelectedEpgTag->ClientID()))
      {
        // default to first channel
        newChannelIndex = 0;
      }
    }

    // restore previous selection.
    if (newChannelIndex == oldChannelIndex && newBlockIndex == oldBlockIndex)
    {
      // same coordinates, keep current grid view port
      UpdateItem();
    }
    else
    {
      // new coordinates, move grid view port accordingly
      SetInvalid();

      if (newBlockIndex != oldBlockIndex)
      {
        m_bEnableProgrammeScrolling = false;
        GoToBlock(newBlockIndex);
      }

      if (newChannelIndex != oldChannelIndex)
      {
        m_bEnableChannelScrolling = false;
        GoToChannel(newChannelIndex);
      }
    }
  }
  else
  {
    // no previous selection, goto now
    SetInvalid();
    m_bEnableProgrammeScrolling = false;
    GoToNow();
  }
}

float CGUIEPGGridContainer::GetChannelScrollOffsetPos() const
{
  if (m_bEnableChannelScrolling)
    return m_channelScrollOffset;
  else
    return m_channelOffset * m_channelLayout->Size(m_orientation);
}

float CGUIEPGGridContainer::GetProgrammeScrollOffsetPos() const
{
  if (m_bEnableProgrammeScrolling)
    return m_programmeScrollOffset;
  else
    return m_blockOffset * m_blockSize;
}

int CGUIEPGGridContainer::GetChannelScrollOffset(CGUIListItemLayout* layout) const
{
  if (m_bEnableChannelScrolling)
    return MathUtils::round_int(
        static_cast<double>(m_channelScrollOffset / layout->Size(m_orientation)));
  else
    return m_channelOffset;
}

int CGUIEPGGridContainer::GetProgrammeScrollOffset() const
{
  if (m_bEnableProgrammeScrolling)
    return MathUtils::round_int(static_cast<double>(m_programmeScrollOffset / m_blockSize));
  else
    return m_blockOffset;
}

void CGUIEPGGridContainer::ChannelScroll(int amount)
{
  // increase or decrease the vertical offset
  int offset = m_channelOffset + amount;

  if (offset > m_gridModel->ChannelItemsSize() - m_channelsPerPage)
    offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

  if (offset < 0)
    offset = 0;

  ScrollToChannelOffset(offset);
  SetChannel(m_channelCursor);
}

void CGUIEPGGridContainer::ProgrammesScroll(int amount)
{
  // increase or decrease the horizontal offset
  ScrollToBlockOffset(m_blockOffset + amount);
  SetBlock(m_blockCursor);
}

void CGUIEPGGridContainer::OnUp()
{
  if (!HasData())
    return CGUIControl::OnUp();

  if (m_orientation == VERTICAL)
  {
    CGUIAction action = GetAction(ACTION_MOVE_UP);
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
      int offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

      if (offset < 0)
        offset = 0;

      SetChannel(m_gridModel->GetLastChannel() - offset);
      ScrollToChannelOffset(offset);
    }
    else
      CGUIControl::OnUp();
  }
  else
  {
    if (m_gridModel->GetGridItemStartBlock(m_channelCursor + m_channelOffset,
                                           m_blockCursor + m_blockOffset) > m_blockOffset)
    {
      // this is not first item on page
      SetItem(GetPrevItem());
      UpdateBlock();
      return;
    }
    else if (m_blockCursor <= 0 && m_blockOffset && m_blockOffset - BLOCK_SCROLL_OFFSET >= 0)
    {
      // this is the first item on page
      ScrollToBlockOffset(m_blockOffset - BLOCK_SCROLL_OFFSET);
      UpdateBlock();
      return;
    }

    CGUIControl::OnUp();
  }
}

void CGUIEPGGridContainer::OnDown()
{
  if (!HasData())
    return CGUIControl::OnDown();

  if (m_orientation == VERTICAL)
  {
    CGUIAction action = GetAction(ACTION_MOVE_DOWN);
    if (m_channelOffset + m_channelCursor < m_gridModel->GetLastChannel())
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
      ScrollToChannelOffset(0);
      SetChannel(0);
    }
    else
      CGUIControl::OnDown();
  }
  else
  {
    if (m_gridModel->GetGridItemEndBlock(m_channelCursor + m_channelOffset,
                                         m_blockCursor + m_blockOffset) <
        (m_blockOffset + m_blocksPerPage - 1))
    {
      // this is not last item on page
      SetItem(GetNextItem());
      UpdateBlock();
      return;
    }
    else if ((m_blockOffset != m_gridModel->GridItemsSize() - m_blocksPerPage) &&
             m_gridModel->GridItemsSize() > m_blocksPerPage &&
             m_blockOffset + BLOCK_SCROLL_OFFSET < m_gridModel->GetLastBlock())
    {
      // this is the last item on page
      ScrollToBlockOffset(m_blockOffset + BLOCK_SCROLL_OFFSET);
      UpdateBlock();
      return;
    }

    CGUIControl::OnDown();
  }
}

void CGUIEPGGridContainer::OnLeft()
{
  if (!HasData())
    return CGUIControl::OnLeft();

  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->GetGridItemStartBlock(m_channelCursor + m_channelOffset,
                                           m_blockCursor + m_blockOffset) > m_blockOffset)
    {
      // this is not first item on page
      SetItem(GetPrevItem());
      UpdateBlock();
      return;
    }
    else if (m_blockCursor <= 0 && m_blockOffset && m_blockOffset - BLOCK_SCROLL_OFFSET >= 0)
    {
      // this is the first item on page
      ScrollToBlockOffset(m_blockOffset - BLOCK_SCROLL_OFFSET);
      UpdateBlock();
      return;
    }

    CGUIControl::OnLeft();
  }
  else
  {
    CGUIAction action = GetAction(ACTION_MOVE_LEFT);
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
      int offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

      if (offset < 0)
        offset = 0;

      SetChannel(m_gridModel->GetLastChannel() - offset);
      ScrollToChannelOffset(offset);
    }
    else
      CGUIControl::OnLeft();
  }
}

void CGUIEPGGridContainer::OnRight()
{
  if (!HasData())
    return CGUIControl::OnRight();

  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->GetGridItemEndBlock(m_channelCursor + m_channelOffset,
                                         m_blockCursor + m_blockOffset) <
        (m_blockOffset + m_blocksPerPage - 1))
    {
      // this is not last item on page
      SetItem(GetNextItem());
      UpdateBlock();
      return;
    }
    else if ((m_blockOffset != m_gridModel->GridItemsSize() - m_blocksPerPage) &&
             m_gridModel->GridItemsSize() > m_blocksPerPage &&
             m_blockOffset + BLOCK_SCROLL_OFFSET < m_gridModel->GetLastBlock())
    {
      // this is the last item on page
      ScrollToBlockOffset(m_blockOffset + BLOCK_SCROLL_OFFSET);
      UpdateBlock();
      return;
    }

    CGUIControl::OnRight();
  }
  else
  {
    CGUIAction action = GetAction(ACTION_MOVE_RIGHT);
    if (m_channelOffset + m_channelCursor < m_gridModel->GetLastChannel())
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
      CGUIControl::OnRight();
  }
}

bool CGUIEPGGridContainer::SetChannel(const std::string& channel)
{
  for (int iIndex = 0; iIndex < m_gridModel->ChannelItemsSize(); iIndex++)
  {
    std::string strPath = m_gridModel->GetChannelItem(iIndex)->GetProperty("path").asString();
    if (strPath == channel)
    {
      GoToChannel(iIndex);
      return true;
    }
  }
  return false;
}

bool CGUIEPGGridContainer::SetChannel(const std::shared_ptr<CPVRChannel>& channel)
{
  for (int iIndex = 0; iIndex < m_gridModel->ChannelItemsSize(); iIndex++)
  {
    int iChannelId = static_cast<int>(m_gridModel->GetChannelItem(iIndex)->GetProperty("channelid").asInteger(-1));
    if (iChannelId == channel->ChannelID())
    {
      GoToChannel(iIndex);
      return true;
    }
  }
  return false;
}

bool CGUIEPGGridContainer::SetChannel(const CPVRChannelNumber& channelNumber)
{
  for (int iIndex = 0; iIndex < m_gridModel->ChannelItemsSize(); iIndex++)
  {
    const CPVRChannelNumber& number =
        m_gridModel->GetChannelItem(iIndex)->GetPVRChannelGroupMemberInfoTag()->ChannelNumber();
    if (number == channelNumber)
    {
      GoToChannel(iIndex);
      return true;
    }
  }
  return false;
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex < m_gridModel->ChannelItemsSize() && blockIndex < m_gridModel->GridItemsSize())
  {
    if (SetItem(m_gridModel->GetGridItem(channelIndex, m_blockTravelAxis), channelIndex,
                m_blockTravelAxis))
    {
      m_channelCursor = channel;
      MarkDirtyRegion();
      UpdateBlock(false);
    }
  }
}

void CGUIEPGGridContainer::SetBlock(int block, bool bUpdateBlockTravelAxis /* = true */)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (block < 0)
    m_blockCursor = 0;
  else if (block > m_blocksPerPage - 1)
    m_blockCursor = m_blocksPerPage - 1;
  else
    m_blockCursor = block;

  if (bUpdateBlockTravelAxis)
    m_blockTravelAxis = m_blockOffset + m_blockCursor;

  UpdateItem();
  MarkDirtyRegion();
}

void CGUIEPGGridContainer::UpdateBlock(bool bUpdateBlockTravelAxis /* = true */)
{
  SetBlock(m_itemStartBlock > 0 ? m_itemStartBlock - m_blockOffset : 0, bUpdateBlockTravelAxis);
}

CGUIListItemLayout* CGUIEPGGridContainer::GetFocusedLayout() const
{
  std::shared_ptr<CGUIListItem> item = GetListItem(0);

  if (item)
    return item->GetFocusedLayout();

  return nullptr;
}

bool CGUIEPGGridContainer::SelectItemFromPoint(const CPoint& point, bool justGrid /* = false */)
{
  /* point has already had origin set to m_posX, m_posY */
  if (!m_focusedProgrammeLayout || !m_programmeLayout || (justGrid && point.x < 0))
    return false;

  int channel;
  int block;

  if (m_orientation == VERTICAL)
  {
    channel = point.y / m_channelHeight;
    block = point.x / m_blockSize;
  }
  else
  {
    channel = point.x / m_channelWidth;
    block = point.y / m_blockSize;
  }

  if (channel > m_channelsPerPage)
    channel = m_channelsPerPage - 1;

  if (channel >= m_gridModel->ChannelItemsSize())
    channel = m_gridModel->GetLastChannel();

  if (channel < 0)
    channel = 0;

  if (block > m_blocksPerPage)
    block = m_blocksPerPage - 1;

  if (block < 0)
    block = 0;

  int channelIndex = channel + m_channelOffset;
  int blockIndex = block + m_blockOffset;

  // bail if out of range
  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GridItemsSize())
    return false;

  // bail if block isn't occupied
  if (!m_gridModel->GetGridItem(channelIndex, blockIndex))
    return false;

  SetChannel(channel);
  SetBlock(block);
  return true;
}

EVENT_RESULT CGUIEPGGridContainer::OnMouseEvent(const CPoint& point,
                                                const MOUSE::CMouseEvent& event)
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
  case ACTION_GESTURE_ABORT:
    {
      // we're done with exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
      ScrollToChannelOffset(MathUtils::round_int(
          static_cast<double>(m_channelScrollOffset / m_channelLayout->Size(m_orientation))));
      SetChannel(m_channelCursor);
      ScrollToBlockOffset(
          MathUtils::round_int(static_cast<double>(m_programmeScrollOffset / m_blockSize)));
      SetBlock(m_blockCursor);
      return EVENT_RESULT_HANDLED;
    }
  case ACTION_GESTURE_PAN:
    {
      m_programmeScrollOffset -= event.m_offsetX;
      m_channelScrollOffset -= event.m_offsetY;

      {
        std::unique_lock<CCriticalSection> lock(m_critSection);

        m_channelOffset = MathUtils::round_int(
            static_cast<double>(m_channelScrollOffset / m_channelLayout->Size(m_orientation)));
        m_blockOffset =
            MathUtils::round_int(static_cast<double>(m_programmeScrollOffset / m_blockSize));
        ValidateOffset();
      }
      return EVENT_RESULT_HANDLED;
    }
  default:
    return EVENT_RESULT_UNHANDLED;
  }
}

bool CGUIEPGGridContainer::OnMouseOver(const CPoint& point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY), false);
  return CGUIControl::OnMouseOver(point);
}

bool CGUIEPGGridContainer::OnMouseClick(int dwButton, const CPoint& point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY)))
  {
    // send click message to window
    OnClick(ACTION_MOUSE_LEFT_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIEPGGridContainer::OnMouseDoubleClick(int dwButton, const CPoint& point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY)))
  {
    // send double click message to window
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
    CGUIListItemLayout* focusedLayout = GetFocusedLayout();

    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }

  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  return SendWindowMessage(msg);
}

bool CGUIEPGGridContainer::OnMouseWheel(char wheel, const CPoint& point)
{
  // doesn't work while an item is selected?
  ProgrammesScroll(-wheel);
  return true;
}

std::shared_ptr<CPVRChannelGroupMember> CGUIEPGGridContainer::GetSelectedChannelGroupMember() const
{
  CFileItemPtr fileItem;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (m_channelCursor + m_channelOffset < m_gridModel->ChannelItemsSize())
      fileItem = m_gridModel->GetChannelItem(m_channelCursor + m_channelOffset);
  }

  if (fileItem)
    return fileItem->GetPVRChannelGroupMemberInfoTag();

  return {};
}

CDateTime CGUIEPGGridContainer::GetSelectedDate() const
{
  return m_gridModel->GetStartTimeForBlock(m_blockOffset + m_blockCursor);
}

CFileItemPtr CGUIEPGGridContainer::GetSelectedGridItem(int offset /*= 0*/) const
{
  CFileItemPtr item;

  if (m_channelCursor + m_channelOffset + offset < m_gridModel->ChannelItemsSize() &&
      m_blockCursor + m_blockOffset < m_gridModel->GridItemsSize())
    item = m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset);

  return item;
}

std::shared_ptr<CGUIListItem> CGUIEPGGridContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_gridModel->HasChannelItems())
    return std::shared_ptr<CGUIListItem>();

  int item = m_channelCursor + m_channelOffset + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION)
    item = GetChannelScrollOffset(m_channelLayout);

  if (flag & INFOFLAG_LISTITEM_WRAP)
  {
    item %= m_gridModel->ChannelItemsSize();
    if (item < 0)
      item += m_gridModel->ChannelItemsSize();

    return m_gridModel->GetChannelItem(item);
  }
  else
  {
    if (item >= 0 && item < m_gridModel->ChannelItemsSize())
      return m_gridModel->GetChannelItem(item);
  }
  return std::shared_ptr<CGUIListItem>();
}

std::string CGUIEPGGridContainer::GetLabel(int info) const
{
  std::string label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    if (m_channelsPerPage > 0)
      label = std::to_string((m_gridModel->ChannelItemsSize() + m_channelsPerPage - 1) /
                             m_channelsPerPage);
    else
      label = std::to_string(0);
    break;
  case CONTAINER_CURRENT_PAGE:
    if (m_channelsPerPage > 0)
      label = std::to_string(1 + (m_channelCursor + m_channelOffset) / m_channelsPerPage);
    else
      label = std::to_string(1);
    break;
  case CONTAINER_POSITION:
    label = std::to_string(1 + m_channelCursor + m_channelOffset);
    break;
  case CONTAINER_NUM_ITEMS:
    label = std::to_string(m_gridModel->ChannelItemsSize());
    break;
  default:
      break;
  }
  return label;
}

void CGUIEPGGridContainer::SetItem(const std::pair<std::shared_ptr<CFileItem>, int>& itemInfo)
{
  SetItem(itemInfo.first, m_channelCursor + m_channelOffset, itemInfo.second);
}

bool CGUIEPGGridContainer::SetItem(const std::shared_ptr<CFileItem>& item,
                                   int channelIndex,
                                   int blockIndex)
{
  if (item && channelIndex < m_gridModel->ChannelItemsSize() &&
      blockIndex < m_gridModel->GridItemsSize())
  {
    m_itemStartBlock = m_gridModel->GetGridItemStartBlock(channelIndex, blockIndex);
    return true;
  }
  else
  {
    m_itemStartBlock = 0;
    return false;
  }
}

std::shared_ptr<CFileItem> CGUIEPGGridContainer::GetItem() const
{
  const int channelIndex = m_channelCursor + m_channelOffset;
  const int blockIndex = m_blockCursor + m_blockOffset;

  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GridItemsSize())
    return {};

  return m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset);
}

std::pair<std::shared_ptr<CFileItem>, int> CGUIEPGGridContainer::GetNextItem() const
{
  int block = m_gridModel->GetGridItemEndBlock(m_channelCursor + m_channelOffset,
                                               m_blockCursor + m_blockOffset);
  if (block < m_gridModel->GridItemsSize())
  {
    // first block of next event is one block after end block of selected event
    block += 1;
  }

  return {m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, block), block};
}

std::pair<std::shared_ptr<CFileItem>, int> CGUIEPGGridContainer::GetPrevItem() const
{
  int block = m_gridModel->GetGridItemStartBlock(m_channelCursor + m_channelOffset,
                                                 m_blockCursor + m_blockOffset);
  if (block > 0)
  {
    // last block of previous event is one block before start block of selected event
    block -= 1;
  }

  return {m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, block), block};
}

void CGUIEPGGridContainer::UpdateItem()
{
  SetItem(GetItem(), m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset);
}

void CGUIEPGGridContainer::SetFocus(bool focus)
{
  if (focus != HasFocus())
    SetInvalid();

  CGUIControl::SetFocus(focus);
}

void CGUIEPGGridContainer::ScrollToChannelOffset(int offset)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  float size = m_programmeLayout->Size(m_orientation);
  int range = m_channelsPerPage / 4;

  if (range <= 0)
    range = 1;

  if (offset * size < m_channelScrollOffset && m_channelScrollOffset - offset * size > size * range)
  {
    // scrolling up, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_channelScrollOffset && offset * size - m_channelScrollOffset > size * range)
  {
    // scrolling down, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset - range) * size;
  }

  m_channelScrollSpeed = (offset * size - m_channelScrollOffset) / m_scrollTime;
  m_channelOffset = offset;
  MarkDirtyRegion();
}

void CGUIEPGGridContainer::ScrollToBlockOffset(int offset)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // make sure offset is in valid range
  offset = std::max(0, std::min(offset, m_gridModel->GridItemsSize() - m_blocksPerPage));

  float size = m_blockSize;
  int range = m_blocksPerPage / 1;

  if (range <= 0)
    range = 1;

  if (offset * size < m_programmeScrollOffset && m_programmeScrollOffset - offset * size > size * range)
  {
    // scrolling left, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_programmeScrollOffset && offset * size - m_programmeScrollOffset > size * range)
  {
    // scrolling right, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset - range) * size;
  }

  m_programmeScrollSpeed = (offset * size - m_programmeScrollOffset) / m_scrollTime;
  m_blockOffset = offset;
  MarkDirtyRegion();
}

void CGUIEPGGridContainer::ValidateOffset()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!m_programmeLayout)
    return;

  float pos = (m_orientation == VERTICAL) ? m_channelHeight : m_channelWidth;

  if (m_gridModel->ChannelItemsSize() &&
      (m_channelOffset > m_gridModel->ChannelItemsSize() - m_channelsPerPage ||
       m_channelScrollOffset > (m_gridModel->ChannelItemsSize() - m_channelsPerPage) * pos))
  {
    m_channelOffset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;
    m_channelScrollOffset = m_channelOffset * pos;
  }

  if (m_channelOffset < 0 || m_channelScrollOffset < 0)
  {
    m_channelOffset = 0;
    m_channelScrollOffset = 0;
  }

  if (m_gridModel->GridItemsSize() &&
      (m_blockOffset > m_gridModel->GridItemsSize() - m_blocksPerPage ||
       m_programmeScrollOffset > (m_gridModel->GridItemsSize() - m_blocksPerPage) * m_blockSize))
  {
    m_blockOffset = m_gridModel->GridItemsSize() - m_blocksPerPage;
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
  }

  if (m_blockOffset < 0 || m_programmeScrollOffset < 0)
  {
    m_blockOffset = 0;
    m_programmeScrollOffset = 0;
  }
}

void CGUIEPGGridContainer::LoadLayout(TiXmlElement* layout)
{
  /* layouts for the channel column */
  TiXmlElement* itemElement = layout->FirstChildElement("channellayout");
  while (itemElement)
  {
    m_channelLayouts.emplace_back();
    m_channelLayouts.back().LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("channellayout");
  }
  itemElement = layout->FirstChildElement("focusedchannellayout");
  while (itemElement)
  {
    m_focusedChannelLayouts.emplace_back();
    m_focusedChannelLayouts.back().LoadLayout(itemElement, GetParentID(), true, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("focusedchannellayout");
  }

  /* layouts for the grid items */
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  {
    m_focusedProgrammeLayouts.emplace_back();
    m_focusedProgrammeLayouts.back().LoadLayout(itemElement, GetParentID(), true, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
  itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  {
    m_programmeLayouts.emplace_back();
    m_programmeLayouts.back().LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }

  /* layout for the date label for the grid */
  itemElement = layout->FirstChildElement("rulerdatelayout");
  while (itemElement)
  {
    m_rulerDateLayouts.emplace_back();
    m_rulerDateLayouts.back().LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("rulerdatelayout");
  }

  /* layout for the timeline for the grid */
  itemElement = layout->FirstChildElement("rulerlayout");
  while (itemElement)
  {
    m_rulerLayouts.emplace_back();
    m_rulerLayouts.back().LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("rulerlayout");
  }

  UpdateLayout();
}

std::string CGUIEPGGridContainer::GetDescription() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const int channelIndex = m_channelCursor + m_channelOffset;
  const int blockIndex = m_blockCursor + m_blockOffset;

  if (channelIndex < m_gridModel->ChannelItemsSize() && blockIndex < m_gridModel->GridItemsSize())
  {
    const std::shared_ptr<CFileItem> item = m_gridModel->GetGridItem(channelIndex, blockIndex);
    if (item)
      return item->GetLabel();
  }

  return {};
}

void CGUIEPGGridContainer::JumpToNow()
{
  m_bEnableProgrammeScrolling = false;
  GoToNow();
}

void CGUIEPGGridContainer::JumpToDate(const CDateTime& date)
{
  m_bEnableProgrammeScrolling = false;
  GoToDate(date);
}

void CGUIEPGGridContainer::GoToBegin()
{
  ScrollToBlockOffset(0);
  SetBlock(0);
}

void CGUIEPGGridContainer::GoToEnd()
{
  ScrollToBlockOffset(m_gridModel->GetLastBlock() - m_blocksPerPage + 1);
  SetBlock(m_blocksPerPage - 1);
}

void CGUIEPGGridContainer::GoToNow()
{
  GoToDate(CDateTime::GetUTCDateTime());
}

void CGUIEPGGridContainer::GoToDate(const CDateTime& date)
{
  unsigned int offset = m_gridModel->GetPageNowOffset();
  ScrollToBlockOffset(m_gridModel->GetBlock(date) - offset);

  // ensure we're selecting the active event, not its predecessor.
  const int iChannel = m_channelOffset + m_channelCursor;
  const int iBlock = m_blockOffset + offset;
  if (iChannel >= m_gridModel->ChannelItemsSize() || iBlock >= m_gridModel->GridItemsSize() ||
      m_gridModel->GetGridItemEndTime(iChannel, iBlock) > date)
  {
    SetBlock(offset);
  }
  else
  {
    SetBlock(offset + 1);
  }
}

void CGUIEPGGridContainer::GoToFirstChannel()
{
  GoToChannel(0);
}

void CGUIEPGGridContainer::GoToLastChannel()
{
  if (m_gridModel->ChannelItemsSize())
    GoToChannel(m_gridModel->GetLastChannel());
  else
    GoToChannel(0);
}

void CGUIEPGGridContainer::GoToTop()
{
  if (m_orientation == VERTICAL)
  {
    GoToChannel(0);
  }
  else
  {
    GoToBlock(0);
  }
}

void CGUIEPGGridContainer::GoToBottom()
{
  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->HasChannelItems())
      GoToChannel(m_gridModel->GetLastChannel());
    else
      GoToChannel(0);
  }
  else
  {
    if (m_gridModel->GridItemsSize())
      GoToBlock(m_gridModel->GetLastBlock());
    else
      GoToBlock(0);
  }
}

void CGUIEPGGridContainer::GoToMostLeft()
{
  if (m_orientation == VERTICAL)
  {
    GoToBlock(0);
  }
  else
  {
    GoToChannel(0);
  }
}

void CGUIEPGGridContainer::GoToMostRight()
{
  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->GridItemsSize())
      GoToBlock(m_gridModel->GetLastBlock());
    else
      GoToBlock(0);
  }
  else
  {
    if (m_gridModel->HasChannelItems())
      GoToChannel(m_gridModel->GetLastChannel());
    else
      GoToChannel(0);
  }
}

void CGUIEPGGridContainer::SetTimelineItems(const std::unique_ptr<CFileItemList>& items,
                                            const CDateTime& gridStart,
                                            const CDateTime& gridEnd)
{
  int iRulerUnit;
  int iFirstChannel;
  int iChannelsPerPage;
  int iBlocksPerPage;
  int iFirstBlock;
  float fBlockSize;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    iRulerUnit = m_rulerUnit;
    iFirstChannel = m_channelOffset;
    iChannelsPerPage = m_channelsPerPage;
    iFirstBlock = m_blockOffset;
    iBlocksPerPage = m_blocksPerPage;
    fBlockSize = m_blockSize;
  }

  std::unique_ptr<CGUIEPGGridContainerModel> oldUpdatedGridModel;
  std::unique_ptr<CGUIEPGGridContainerModel> newUpdatedGridModel(new CGUIEPGGridContainerModel);

  newUpdatedGridModel->Initialize(items, gridStart, gridEnd, iFirstChannel, iChannelsPerPage,
                                  iFirstBlock, iBlocksPerPage, iRulerUnit, fBlockSize);
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    // grid contains CFileItem instances. CFileItem dtor locks global graphics mutex.
    // by increasing its refcount make sure, old data are not deleted while we're holding own mutex.
    oldUpdatedGridModel = std::move(m_updatedGridModel);

    m_updatedGridModel = std::move(newUpdatedGridModel);
  }
}

std::unique_ptr<CFileItemList> CGUIEPGGridContainer::GetCurrentTimeLineItems() const
{
  return m_gridModel->GetCurrentTimeLineItems(m_channelOffset, m_channelsPerPage);
}

void CGUIEPGGridContainer::GoToChannel(int channelIndex)
{
  if (channelIndex < m_channelsPerPage)
  {
    // first page
    ScrollToChannelOffset(0);
    SetChannel(channelIndex);
  }
  else if (channelIndex > m_gridModel->ChannelItemsSize() - m_channelsPerPage)
  {
    // last page
    ScrollToChannelOffset(m_gridModel->ChannelItemsSize() - m_channelsPerPage);
    SetChannel(channelIndex - (m_gridModel->ChannelItemsSize() - m_channelsPerPage));
  }
  else
  {
    ScrollToChannelOffset(channelIndex - m_channelCursor);
    SetChannel(m_channelCursor);
  }
}

void CGUIEPGGridContainer::GoToBlock(int blockIndex)
{
  int lastPage = m_gridModel->GridItemsSize() - m_blocksPerPage;
  if (blockIndex > lastPage)
  {
    // last page
    ScrollToBlockOffset(lastPage);
    SetBlock(blockIndex - lastPage);
  }
  else
  {
    ScrollToBlockOffset(blockIndex - m_blockCursor);
    SetBlock(m_blockCursor);
  }
}

void CGUIEPGGridContainer::UpdateLayout()
{
  CGUIListItemLayout* oldFocusedChannelLayout = m_focusedChannelLayout;
  CGUIListItemLayout* oldChannelLayout = m_channelLayout;
  CGUIListItemLayout* oldFocusedProgrammeLayout = m_focusedProgrammeLayout;
  CGUIListItemLayout* oldProgrammeLayout = m_programmeLayout;
  CGUIListItemLayout* oldRulerLayout = m_rulerLayout;
  CGUIListItemLayout* oldRulerDateLayout = m_rulerDateLayout;

  GetCurrentLayouts();

  // Note: m_rulerDateLayout is optional
  if (!m_focusedProgrammeLayout || !m_programmeLayout || !m_focusedChannelLayout || !m_channelLayout || !m_rulerLayout)
    return;

  if (oldChannelLayout == m_channelLayout && oldFocusedChannelLayout == m_focusedChannelLayout &&
      oldProgrammeLayout == m_programmeLayout && oldFocusedProgrammeLayout == m_focusedProgrammeLayout &&
      oldRulerLayout == m_rulerLayout && oldRulerDateLayout == m_rulerDateLayout)
    return; // nothing has changed, so don't update stuff

  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_channelHeight = m_channelLayout->Size(VERTICAL);
  m_channelWidth = m_channelLayout->Size(HORIZONTAL);

  m_rulerDateHeight = m_rulerDateLayout ? m_rulerDateLayout->Size(VERTICAL) : 0;
  m_rulerDateWidth = m_rulerDateLayout ? m_rulerDateLayout->Size(HORIZONTAL) : 0;

  if (m_orientation == VERTICAL)
  {
    m_rulerHeight = m_rulerLayout->Size(VERTICAL);
    m_gridPosX = m_posX + m_channelWidth;
    m_gridPosY = m_posY + m_rulerHeight + m_rulerDateHeight;
    m_gridWidth = m_width - m_channelWidth;
    m_gridHeight = m_height - m_rulerHeight - m_rulerDateHeight;
    m_blockSize = m_gridWidth / m_blocksPerPage;
    m_rulerWidth = m_rulerUnit * m_blockSize;
    m_channelPosX = m_posX;
    m_channelPosY = m_posY + m_rulerHeight + m_rulerDateHeight;
    m_rulerPosX = m_posX + m_channelWidth;
    m_rulerPosY = m_posY + m_rulerDateHeight;
    m_channelsPerPage = m_gridHeight / m_channelHeight;
    m_programmesPerPage = (m_gridWidth / m_blockSize) + 1;

    m_programmeLayout->SetHeight(m_channelHeight);
    m_focusedProgrammeLayout->SetHeight(m_channelHeight);
  }
  else
  {
    m_rulerWidth = m_rulerLayout->Size(HORIZONTAL);
    m_gridPosX = m_posX + m_rulerWidth;
    m_gridPosY = m_posY + m_channelHeight + m_rulerDateHeight;
    m_gridWidth = m_width - m_rulerWidth;
    m_gridHeight = m_height - m_channelHeight - m_rulerDateHeight;
    m_blockSize = m_gridHeight / m_blocksPerPage;
    m_rulerHeight = m_rulerUnit * m_blockSize;
    m_channelPosX = m_posX + m_rulerWidth;
    m_channelPosY = m_posY + m_rulerDateHeight;
    m_rulerPosX = m_posX;
    m_rulerPosY = m_posY + m_channelHeight + m_rulerDateHeight;
    m_channelsPerPage = m_gridWidth / m_channelWidth;
    m_programmesPerPage = (m_gridHeight / m_blockSize) + 1;

    m_programmeLayout->SetWidth(m_channelWidth);
    m_focusedProgrammeLayout->SetWidth(m_channelWidth);
  }

  // ensure that the scroll offsets are a multiple of our sizes
  m_channelScrollOffset = m_channelOffset * m_programmeLayout->Size(m_orientation);
  m_programmeScrollOffset = m_blockOffset * m_blockSize;
}

void CGUIEPGGridContainer::UpdateScrollOffset(unsigned int currentTime)
{
  if (!m_programmeLayout)
    return;

  m_channelScrollOffset += m_channelScrollSpeed * (currentTime - m_channelScrollLastTime);
  if ((m_channelScrollSpeed < 0 && m_channelScrollOffset < m_channelOffset * m_programmeLayout->Size(m_orientation)) ||
      (m_channelScrollSpeed > 0 && m_channelScrollOffset > m_channelOffset * m_programmeLayout->Size(m_orientation)))
  {
    m_channelScrollOffset = m_channelOffset * m_programmeLayout->Size(m_orientation);
    m_channelScrollSpeed = 0;
    m_bEnableChannelScrolling = true;
  }

  m_channelScrollLastTime = currentTime;
  m_programmeScrollOffset += m_programmeScrollSpeed * (currentTime - m_programmeScrollLastTime);

  if ((m_programmeScrollSpeed < 0 && m_programmeScrollOffset < m_blockOffset * m_blockSize) ||
      (m_programmeScrollSpeed > 0 && m_programmeScrollOffset > m_blockOffset * m_blockSize))
  {
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
    m_programmeScrollSpeed = 0;
    m_bEnableProgrammeScrolling = true;
  }

  m_programmeScrollLastTime = currentTime;

  if (m_channelScrollSpeed || m_programmeScrollSpeed)
    MarkDirtyRegion();
}

void CGUIEPGGridContainer::GetCurrentLayouts()
{
  m_channelLayout = nullptr;

  for (unsigned int i = 0; i < m_channelLayouts.size(); i++)
  {
    if (m_channelLayouts[i].CheckCondition())
    {
      m_channelLayout = &m_channelLayouts[i];
      break;
    }
  }

  if (!m_channelLayout && !m_channelLayouts.empty())
    m_channelLayout = &m_channelLayouts[0]; // failsafe

  m_focusedChannelLayout = nullptr;

  for (unsigned int i = 0; i < m_focusedChannelLayouts.size(); i++)
  {
    if (m_focusedChannelLayouts[i].CheckCondition())
    {
      m_focusedChannelLayout = &m_focusedChannelLayouts[i];
      break;
    }
  }

  if (!m_focusedChannelLayout && !m_focusedChannelLayouts.empty())
    m_focusedChannelLayout = &m_focusedChannelLayouts[0]; // failsafe

  m_programmeLayout = nullptr;

  for (unsigned int i = 0; i < m_programmeLayouts.size(); i++)
  {
    if (m_programmeLayouts[i].CheckCondition())
    {
      m_programmeLayout = &m_programmeLayouts[i];
      break;
    }
  }

  if (!m_programmeLayout && !m_programmeLayouts.empty())
    m_programmeLayout = &m_programmeLayouts[0]; // failsafe

  m_focusedProgrammeLayout = nullptr;

  for (unsigned int i = 0; i < m_focusedProgrammeLayouts.size(); i++)
  {
    if (m_focusedProgrammeLayouts[i].CheckCondition())
    {
      m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[i];
      break;
    }
  }

  if (!m_focusedProgrammeLayout && !m_focusedProgrammeLayouts.empty())
    m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[0]; // failsafe

  m_rulerLayout = nullptr;

  for (unsigned int i = 0; i < m_rulerLayouts.size(); i++)
  {
    if (m_rulerLayouts[i].CheckCondition())
    {
      m_rulerLayout = &m_rulerLayouts[i];
      break;
    }
  }

  if (!m_rulerLayout && !m_rulerLayouts.empty())
    m_rulerLayout = &m_rulerLayouts[0]; // failsafe

  m_rulerDateLayout = nullptr;

  for (unsigned int i = 0; i < m_rulerDateLayouts.size(); i++)
  {
    if (m_rulerDateLayouts[i].CheckCondition())
    {
      m_rulerDateLayout = &m_rulerDateLayouts[i];
      break;
    }
  }

  // Note: m_rulerDateLayout is optional; so no "failsafe" logic here (see above)
}

void CGUIEPGGridContainer::SetRenderOffset(const CPoint& offset)
{
  m_renderOffset = offset;
}

void CGUIEPGGridContainer::GetChannelCacheOffsets(int& cacheBefore, int& cacheAfter)
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

void CGUIEPGGridContainer::GetProgrammeCacheOffsets(int& cacheBefore, int& cacheAfter)
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

void CGUIEPGGridContainer::HandleChannels(bool bRender,
                                          unsigned int currentTime,
                                          CDirtyRegionList& dirtyregions,
                                          bool bAssignDepth)
{
  if (!m_focusedChannelLayout || !m_channelLayout)
    return;

  const int chanOffset = GetChannelScrollOffset(m_programmeLayout);

  int cacheBeforeChannel, cacheAfterChannel;
  GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

  if (bRender)
  {
    if (m_orientation == VERTICAL)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_channelPosX, m_channelPosY, m_channelWidth, m_gridHeight);
    else
      CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_channelPosX, m_channelPosY, m_gridWidth, m_channelHeight);
  }
  else
  {
    // Free memory not used on screen
    if (m_gridModel->ChannelItemsSize() > m_channelsPerPage + cacheBeforeChannel + cacheAfterChannel)
      m_gridModel->FreeChannelMemory(chanOffset - cacheBeforeChannel,
                                     chanOffset + m_channelsPerPage - 1 + cacheAfterChannel);
  }

  CPoint originChannel = CPoint(m_channelPosX, m_channelPosY) + m_renderOffset;
  float pos;
  float end;

  if (m_orientation == VERTICAL)
  {
    pos = originChannel.y;
    end = m_posY + m_height;
  }
  else
  {
    pos = originChannel.x;
    end = m_posX + m_width;
  }

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (chanOffset - cacheBeforeChannel) * m_channelLayout->Size(m_orientation) -
                     GetChannelScrollOffsetPos();
  if (m_channelOffset + m_channelCursor < chanOffset)
    drawOffset += m_focusedChannelLayout->Size(m_orientation) - m_channelLayout->Size(m_orientation);

  pos += drawOffset;
  end += cacheAfterChannel * m_channelLayout->Size(m_orientation);

  float focusedPos = 0;
  std::shared_ptr<CGUIListItem> focusedItem;

  CFileItemPtr item;
  int current = chanOffset - cacheBeforeChannel;
  while (pos < end && m_gridModel->HasChannelItems())
  {
    int itemNo = current;
    if (itemNo >= m_gridModel->ChannelItemsSize())
      break;

    bool focused = (current == m_channelOffset + m_channelCursor);
    if (itemNo >= 0)
    {
      item = m_gridModel->GetChannelItem(itemNo);
      if (bRender)
      {
        // render our item
        if (focused)
        {
          focusedPos = pos;
          focusedItem = item;
        }
        else
        {
          if (m_orientation == VERTICAL)
            RenderItem(originChannel.x, pos, item.get(), false);
          else
            RenderItem(pos, originChannel.y, item.get(), false);
        }
      }
      else if (bAssignDepth)
      {
        if (focused)
          focusedItem = item;
        else
          AssignItemDepth(item.get(), false);
      }
      else
      {
        // process our item
        if (m_orientation == VERTICAL)
          ProcessItem(originChannel.x, pos, item, m_lastItem, focused, m_channelLayout, m_focusedChannelLayout, currentTime, dirtyregions);
        else
          ProcessItem(pos, originChannel.y, item, m_lastItem, focused, m_channelLayout, m_focusedChannelLayout, currentTime, dirtyregions);
      }
    }
    // increment our position
    pos += focused ? m_focusedChannelLayout->Size(m_orientation) : m_channelLayout->Size(m_orientation);
    current++;
  }

  if (bRender)
  {
    // render focused item last so it can overlap other items
    if (focusedItem)
    {
      if (m_orientation == VERTICAL)
        RenderItem(originChannel.x, focusedPos, focusedItem.get(), true);
      else
        RenderItem(focusedPos, originChannel.y, focusedItem.get(), true);
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  }
  else if (bAssignDepth && focusedItem)
  {
    AssignItemDepth(focusedItem.get(), true);
  }
}

void CGUIEPGGridContainer::HandleRulerDate(bool bRender,
                                           unsigned int currentTime,
                                           CDirtyRegionList& dirtyregions,
                                           bool bAssignDepth)
{
  if (!m_rulerDateLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  CFileItemPtr item(m_gridModel->GetRulerItem(0));

  if (bRender)
  {
    // Render single ruler item with date of selected programme
    CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_posX, m_posY, m_rulerDateWidth, m_rulerDateHeight);
    RenderItem(m_posX, m_posY, item.get(), false);
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  }
  else if (bAssignDepth)
  {
    AssignItemDepth(item.get(), false);
  }
  else
  {
    const int rulerOffset = GetProgrammeScrollOffset();
    item->SetLabel(m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1)->GetLabel2());

    CFileItemPtr lastitem;
    ProcessItem(m_posX, m_posY, item, lastitem, false, m_rulerDateLayout, m_rulerDateLayout, currentTime, dirtyregions);
  }
}

void CGUIEPGGridContainer::HandleRuler(bool bRender,
                                       unsigned int currentTime,
                                       CDirtyRegionList& dirtyregions,
                                       bool bAssignDepth)
{
  if (!m_rulerLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  int rulerOffset = GetProgrammeScrollOffset();

  CFileItemPtr item(m_gridModel->GetRulerItem(0));
  CFileItemPtr lastitem;
  int cacheBeforeRuler, cacheAfterRuler;

  if (bRender)
  {
    if (!m_rulerDateLayout)
    {
      // Render single ruler item with date of selected programme
      CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_posX, m_posY, m_width, m_height);
      RenderItem(m_posX, m_posY, item.get(), false);
      CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
    }

    // render ruler items
    GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

    if (m_orientation == VERTICAL)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_rulerPosX, m_rulerPosY, m_gridWidth, m_rulerHeight);
    else
      CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_rulerPosX, m_rulerPosY, m_rulerWidth, m_gridHeight);
  }
  else if (bAssignDepth)
  {
    if (!m_rulerDateLayout)
      AssignItemDepth(item.get(), false);
    GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);
  }
  else
  {
    if (!m_rulerDateLayout)
    {
      item->SetLabel(m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1)->GetLabel2());
      ProcessItem(m_posX, m_posY, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_channelWidth);
    }

    GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

    // Free memory not used on screen
    if (m_gridModel->RulerItemsSize() > m_blocksPerPage + cacheBeforeRuler + cacheAfterRuler)
      m_gridModel->FreeRulerMemory(rulerOffset / m_rulerUnit + 1 - cacheBeforeRuler,
                                   rulerOffset / m_rulerUnit + 1 + m_blocksPerPage - 1 +
                                       cacheAfterRuler);
  }

  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  float pos;
  float end;

  if (m_orientation == VERTICAL)
  {
    pos = originRuler.x;
    end = m_posX + m_width;
  }
  else
  {
    pos = originRuler.y;
    end = m_posY + m_height;
  }

  const float drawOffset =
      (rulerOffset - cacheBeforeRuler) * m_blockSize - GetProgrammeScrollOffsetPos();
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

  while (pos < end && (rulerOffset / m_rulerUnit + 1) < m_gridModel->RulerItemsSize())
  {
    item = m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1);

    if (m_orientation == VERTICAL)
    {
      if (bRender)
        RenderItem(pos, originRuler.y, item.get(), false);
      else if (bAssignDepth)
        AssignItemDepth(item.get(), false);
      else
        ProcessItem(pos, originRuler.y, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_rulerWidth);

      pos += m_rulerWidth;
    }
    else
    {
      if (bRender)
        RenderItem(originRuler.x, pos, item.get(), false);
      else if (bAssignDepth)
        AssignItemDepth(item.get(), false);
      else
        ProcessItem(originRuler.x, pos, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_rulerHeight);

      pos += m_rulerHeight;
    }

    rulerOffset += m_rulerUnit;
  }

  if (bRender)
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
}

void CGUIEPGGridContainer::HandleProgrammeGrid(bool bRender,
                                               unsigned int currentTime,
                                               CDirtyRegionList& dirtyregions,
                                               bool bAssignDepth)
{
  if (!m_focusedProgrammeLayout || !m_programmeLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  const int blockOffset = GetProgrammeScrollOffset();
  const int chanOffset = GetChannelScrollOffset(m_programmeLayout);

  int cacheBeforeProgramme, cacheAfterProgramme;
  GetProgrammeCacheOffsets(cacheBeforeProgramme, cacheAfterProgramme);

  if (bRender)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_gridPosX, m_gridPosY, m_gridWidth, m_gridHeight);
  }
  else if (!bAssignDepth)
  {
    int cacheBeforeChannel, cacheAfterChannel;
    GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

    // Free memory not used on screen
    int firstChannel = chanOffset - cacheBeforeChannel;
    if (firstChannel < 0)
      firstChannel = 0;
    int lastChannel = chanOffset + m_channelsPerPage - 1 + cacheAfterChannel;
    if (lastChannel > m_gridModel->GetLastChannel())
      lastChannel = m_gridModel->GetLastChannel();
    int firstBlock = blockOffset - cacheBeforeProgramme;
    if (firstBlock < 0)
      firstBlock = 0;
    int lastBlock = blockOffset + m_programmesPerPage - 1 + cacheAfterProgramme;
    if (lastBlock > m_gridModel->GetLastBlock())
      lastBlock = m_gridModel->GetLastBlock();

    if (m_gridModel->FreeProgrammeMemory(firstChannel, lastChannel, firstBlock, lastBlock))
    {
      // announce changed viewport
      const CGUIMessage msg(
          GUI_MSG_REFRESH_LIST, GetParentID(), GetID(), static_cast<int>(PVREvent::Epg));
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg);
    }
  }

  CPoint originProgramme = CPoint(m_gridPosX, m_gridPosY) + m_renderOffset;
  float posA;
  float endA;
  float posB;
  float endB;

  if (m_orientation == VERTICAL)
  {
    posA = originProgramme.x;
    endA = m_posX + m_width;
    posB = originProgramme.y;
    endB = m_gridPosY + m_gridHeight;
  }
  else
  {
    posA = originProgramme.y;
    endA = m_posY + m_height;
    posB = originProgramme.x;
    endB = m_gridPosX + m_gridWidth;
  }

  endA += cacheAfterProgramme * m_blockSize;

  const float drawOffsetA = blockOffset * m_blockSize - GetProgrammeScrollOffsetPos();
  posA += drawOffsetA;
  const float drawOffsetB =
      (chanOffset - cacheBeforeProgramme) * m_channelLayout->Size(m_orientation) -
      GetChannelScrollOffsetPos();
  posB += drawOffsetB;

  int channel = chanOffset - cacheBeforeProgramme;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CFileItemPtr focusedItem;
  CFileItemPtr item;

  const int lastChannel = m_gridModel->GetLastChannel();
  while (posB < endB && HasData() && channel <= lastChannel)
  {
    if (channel >= 0)
    {
      int block = blockOffset;
      float posA2 = posA;

      const int startBlock = blockOffset == 0 ? 0 : blockOffset - 1;
      if (startBlock == 0 || m_gridModel->IsSameGridItem(channel, block, startBlock))
      {
        // First program starts before current view
        block = m_gridModel->GetGridItemStartBlock(channel, startBlock);
        const int missingSection = blockOffset - block;
        posA2 -= missingSection * m_blockSize;
      }

      const int lastBlock = m_gridModel->GetLastBlock();
      while (posA2 < endA && HasData() && block <= lastBlock)
      {
        item = m_gridModel->GetGridItem(channel, block);

        bool focused = (channel == m_channelOffset + m_channelCursor) &&
                       m_gridModel->IsSameGridItem(m_channelOffset + m_channelCursor,
                                                   m_blockOffset + m_blockCursor, block);

        if (bRender)
        {
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
            if (m_orientation == VERTICAL)
              RenderItem(posA2, posB, item.get(), focused);
            else
              RenderItem(posB, posA2, item.get(), focused);
          }
        }
        else if (bAssignDepth)
        {
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
            AssignItemDepth(item.get(), focused);
          }
        }
        else
        {
          // calculate the size to truncate if item is out of grid view
          float truncateSize = 0;
          if (posA2 < posA)
          {
            truncateSize = posA - posA2;
            posA2 = posA; // reset to grid start position
          }

          {
            std::unique_lock<CCriticalSection> lock(m_critSection);
            // truncate item's width
            m_gridModel->DecreaseGridItemWidth(channel, block, truncateSize);
          }

          if (m_orientation == VERTICAL)
            ProcessItem(posA2, posB, item, m_lastChannel, focused, m_programmeLayout,
                        m_focusedProgrammeLayout, currentTime, dirtyregions,
                        m_gridModel->GetGridItemWidth(channel, block));
          else
            ProcessItem(posB, posA2, item, m_lastChannel, focused, m_programmeLayout,
                        m_focusedProgrammeLayout, currentTime, dirtyregions,
                        m_gridModel->GetGridItemWidth(channel, block));
        }

        // increment our X position
        posA2 += m_gridModel->GetGridItemWidth(
            channel, block); // assumes focused & unfocused layouts have equal length
        block += MathUtils::round_int(
            static_cast<double>(m_gridModel->GetGridItemOriginWidth(channel, block) / m_blockSize));
      }
    }

    // increment our Y position
    channel++;
    posB += (m_orientation == VERTICAL) ? m_channelHeight : m_channelWidth;
  }

  if (bRender)
  {
    // and render the focused item last (for overlapping purposes)
    if (focusedItem)
    {
      if (m_orientation == VERTICAL)
        RenderItem(focusedPosX, focusedPosY, focusedItem.get(), true);
      else
        RenderItem(focusedPosY, focusedPosX, focusedItem.get(), true);
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  }
  else if (bAssignDepth && focusedItem)
  {
    AssignItemDepth(focusedItem.get(), true);
  }
}

void CGUIEPGGridContainer::AssignDepth()
{
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleChannels(false, dummyTime, dummyRegions, true);
  HandleRuler(false, dummyTime, dummyRegions, true);
  HandleRulerDate(false, dummyTime, dummyRegions, true);
  HandleProgrammeGrid(false, dummyTime, dummyRegions, true);
  m_guiProgressIndicatorTextureDepth = CServiceBroker::GetWinSystem()->GetGfxContext().GetDepth();
}

void CGUIEPGGridContainer::AssignItemDepth(CGUIListItem* item, bool focused)
{
  if (focused)
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->AssignDepth();
  }
  else
  {
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->AssignDepth();
    else if (item->GetLayout())
      item->GetLayout()->AssignDepth();
  }
}