/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogTeletext.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUITexture.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/Texture.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ColorUtils.h"
#include "utils/log.h"

static int teletextFadeAmount = 0;

CGUIDialogTeletext::CGUIDialogTeletext()
  : CGUIDialog(WINDOW_DIALOG_OSD_TELETEXT, ""), m_pTxtTexture(nullptr)
{
  m_renderOrder = RENDER_ORDER_DIALOG_TELETEXT;
}

CGUIDialogTeletext::~CGUIDialogTeletext() = default;

bool CGUIDialogTeletext::OnAction(const CAction& action)
{
  if (m_TextDecoder.HandleAction(action))
  {
    MarkDirtyRegion();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTeletext::OnBack(int actionID)
{
  m_bClose = true;
  MarkDirtyRegion();
  return true;
}

bool CGUIDialogTeletext::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    /* Do not open if no teletext is available */
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (!appPlayer->HasTeletextCache())
    {
      Close();
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(23049), "", 1500, false);
      return true;
    }
  }
  else if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
    {
      SetCoordinates();
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTeletext::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_TextDecoder.Changed())
  {
    MarkDirtyRegion();
  }
  CGUIDialog::Process(currentTime, dirtyregions);
  m_renderRegion = m_vertCoords;
}

void CGUIDialogTeletext::Render()
{
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;
  // Do not render if we have no texture
  if (!m_pTxtTexture)
  {
    CLog::Log(LOGERROR, "CGUITeletextBox::Render called without texture");
    return;
  }

  m_TextDecoder.RenderPage();

  if (!m_bClose)
  {
    if (teletextFadeAmount < 100)
    {
      teletextFadeAmount = std::min(100, teletextFadeAmount + 5);
      MarkDirtyRegion();
    }
  }
  else
  {
    if (teletextFadeAmount > 0)
    {
      teletextFadeAmount = std::max(0, teletextFadeAmount - 10);
      MarkDirtyRegion();
    }

    if (teletextFadeAmount == 0)
      Close();
  }

  unsigned char* textureBuffer = (unsigned char*)m_TextDecoder.GetTextureBuffer();
  if (!m_bClose && m_TextDecoder.NeedRendering() && textureBuffer)
  {
    m_pTxtTexture->Update(m_TextDecoder.GetWidth(), m_TextDecoder.GetHeight(), m_TextDecoder.GetWidth()*4, XB_FMT_A8R8G8B8, textureBuffer, false);
    m_TextDecoder.RenderingDone();
    MarkDirtyRegion();
  }

  UTILS::COLOR::Color color =
      (static_cast<UTILS::COLOR::Color>(teletextFadeAmount * 2.55f) & 0xff) << 24 | 0xFFFFFF;
  CGUITexture::DrawQuad(m_vertCoords, color, m_pTxtTexture.get(), nullptr, -1.0f);

  CGUIDialog::Render();
}

void CGUIDialogTeletext::OnInitWindow()
{
  teletextFadeAmount  = 0;
  m_bClose            = false;
  m_windowLoaded      = true;

  SetCoordinates();

  if (!m_TextDecoder.InitDecoder())
  {
    CLog::Log(LOGERROR, "{}: failed to init teletext decoder", __FUNCTION__);
    Close();
  }

  m_pTxtTexture =
      CTexture::CreateTexture(m_TextDecoder.GetWidth(), m_TextDecoder.GetHeight(), XB_FMT_A8R8G8B8);
  if (!m_pTxtTexture)
  {
    CLog::Log(LOGERROR, "{}: failed to create texture", __FUNCTION__);
    Close();
  }

  CGUIDialog::OnInitWindow();
}

void CGUIDialogTeletext::OnDeinitWindow(int nextWindowID)
{
  m_windowLoaded = false;
  m_TextDecoder.EndDecoder();

  m_pTxtTexture.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIDialogTeletext::SetCoordinates()
{
  float left, right, top, bottom;

  CServiceBroker::GetWinSystem()->GetGfxContext().SetScalingResolution(m_coordsRes, m_needsScaling);

  left = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(0, 0);
  right = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord((float)m_coordsRes.iWidth, 0);
  top = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(0, 0);
  bottom = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(0, (float)m_coordsRes.iHeight);

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_TELETEXTSCALE))
  {
    /* Fixed aspect ratio to 4:3 for teletext */
    float width = right - left;
    float height = bottom - top;
    if (width / 4 > height / 3)
    {
      left = (width - height * 4 / 3) / 2;
      right = width - left;
    }
    else
    {
      top = (height - width * 3 / 4) / 2;
      bottom = height - top;
    }
  }

  m_vertCoords.SetRect(left,
    top,
    right,
    bottom);

  MarkDirtyRegion();
}
