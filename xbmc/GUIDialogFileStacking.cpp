#include "stdafx.h"
#include "GUIDialogFileStacking.h"


CGUIDialogFileStacking::CGUIDialogFileStacking(void)
    : CGUIDialog(WINDOW_DIALOG_FILESTACKING, "DialogFileStacking.xml")
{
  m_iSelectedFile = -1;
  m_iNumberOfFiles = 0;
  m_dwTimerTick = 0;
}

CGUIDialogFileStacking::~CGUIDialogFileStacking(void)
{}

bool CGUIDialogFileStacking::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // skip if set to immediately
      // just pick first file and close
      if (g_guiSettings.GetInt("VideoPlayer.BypassCDSelection") == 1)
      {
        m_iSelectedFile = 1;
        Close();
        return true;
      }

      CGUIDialog::OnMessage(message);
      m_iSelectedFile = -1;
      m_iFrames = 0;

      // enable the CD's
      for (int i = 1; i <= m_iNumberOfFiles; ++i)
      {
        CONTROL_ENABLE(i);
        SET_CONTROL_VISIBLE(i);
      }

      // disable CD's we dont use
      for (int i = m_iNumberOfFiles + 1; i <= 40; ++i)
      {
        SET_CONTROL_HIDDEN(i);
        CONTROL_DISABLE(i);
      }

      m_dwTimerTick = timeGetTime();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      m_iSelectedFile = message.GetSenderId();
      Close();
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

int CGUIDialogFileStacking::GetSelectedFile() const
{
  return m_iSelectedFile;
}

void CGUIDialogFileStacking::SetNumberOfFiles(int iFiles)
{
  m_iNumberOfFiles = iFiles;
}

void CGUIDialogFileStacking::Render()
{
  if (m_iFrames <= 25)
  {
    // slide in...
    int dwScreenWidth = g_graphicsContext.GetWidth();
    for (int i = 1; i <= m_iNumberOfFiles; ++i)
    {
      CGUIControl* pControl = (CGUIControl*)GetControl(i);
      if (pControl)
      {
        DWORD dwEndPos = dwScreenWidth - ((m_iNumberOfFiles - i) * 32) - 140;
        DWORD dwStartPos = dwScreenWidth;
        float fStep = (float)(dwStartPos - dwEndPos);
        fStep /= 25.0f;
        fStep *= (float)m_iFrames;
        DWORD dwPosX = (DWORD) ( ((float)dwStartPos) - fStep );
        pControl->SetPosition( dwPosX, pControl->GetYPosition() );
      }
    }
    m_iFrames++;
  }
  CGUIDialog::Render();

  // now check timer, 5-180 seconds
  int iTimerSetting = g_guiSettings.GetInt("VideoPlayer.BypassCDSelection");
  if (iTimerSetting > 1)
  {
    iTimerSetting--;
    iTimerSetting *= 5;
    if ((long)(timeGetTime() - m_dwTimerTick) >= (long)(iTimerSetting*1000L))
    {
      m_iSelectedFile = 1;
      Close();
    }
  }
}