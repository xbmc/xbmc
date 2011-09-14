/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogTeletext.h"
#include "utils/log.h"
#include "settings/GUISettings.h"
#include "Application.h"
#include "guilib/GUITexture.h"
#include "guilib/Texture.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"

using namespace std;

static int teletextFadeAmount = 0;

CGUIDialogTeletext::CGUIDialogTeletext()
    : CGUIDialog(WINDOW_DIALOG_OSD_TELETEXT, "")
{
  m_isDialog    = false;
  m_pTxtTexture = NULL;
}

CGUIDialogTeletext::~CGUIDialogTeletext()
{
}

bool CGUIDialogTeletext::OnAction(const CAction& action)
{
  if (m_TextDecoder.HandleAction(action))
    return true;

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTeletext::OnBack(int actionID)
{
  m_bClose = true;
  return true;
}

bool CGUIDialogTeletext::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    /* Do not open if no teletext is available */
    if (!g_application.m_pPlayer->GetTeletextCache())
    {
      Close();
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(23049), "", 1500, false);
      return true;
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTeletext::Render()
{
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
      teletextFadeAmount = std::min(100, teletextFadeAmount + 5);
  }
  else
  {
    if (teletextFadeAmount > 0)
      teletextFadeAmount = std::max(0, teletextFadeAmount - 10);

    if (teletextFadeAmount == 0)
      Close();
  }

  unsigned char* textureBuffer = (unsigned char*)m_TextDecoder.GetTextureBuffer();
  if (!m_bClose && m_TextDecoder.NeedRendering() && textureBuffer)
  {
    m_pTxtTexture->Update(m_TextDecoder.GetWidth(), m_TextDecoder.GetHeight(), m_TextDecoder.GetWidth()*4, XB_FMT_A8R8G8B8, textureBuffer, false);
    m_TextDecoder.RenderingDone();
  }

  color_t color = ((color_t)(teletextFadeAmount * 2.55f) & 0xff) << 24 | 0xFFFFFF;
  CGUITexture::DrawQuad(m_vertCoords, color, m_pTxtTexture);

  CGUIDialog::Render();
}

void CGUIDialogTeletext::OnInitWindow()
{
  teletextFadeAmount  = 0;
  m_bClose            = false;
  m_windowLoaded      = true;

  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  m_vertCoords.SetRect((float)g_settings.m_ResInfo[res].Overscan.left,
                       (float)g_settings.m_ResInfo[res].Overscan.top,
                       (float)g_settings.m_ResInfo[res].Overscan.right,
                       (float)g_settings.m_ResInfo[res].Overscan.bottom);

  if (!m_TextDecoder.InitDecoder())
  {
    CLog::Log(LOGERROR, "%s: failed to init teletext decoder", __FUNCTION__);
    Close();
  }

  m_pTxtTexture = new CTexture(m_TextDecoder.GetWidth(), m_TextDecoder.GetHeight(), XB_FMT_A8R8G8B8);
  if (!m_pTxtTexture)
  {
    CLog::Log(LOGERROR, "%s: failed to create texture", __FUNCTION__);
    Close();
  }

  CGUIDialog::OnInitWindow();
}

void CGUIDialogTeletext::OnDeinitWindow(int nextWindowID)
{
  m_windowLoaded = false;
  m_TextDecoder.EndDecoder();

  delete m_pTxtTexture;
  m_pTxtTexture = NULL;

  CGUIDialog::OnDeinitWindow(nextWindowID);
}
