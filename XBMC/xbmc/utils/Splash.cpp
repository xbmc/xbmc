
#include "stdafx.h"
#include "Splash.h"
#include "guiImage.h"
#include "..\util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSplash::CSplash(const CStdString& imageName)
{
  m_ImageName = imageName;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Process()
{
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;

  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  int w = g_graphicsContext.GetWidth() / 2;
  int h = g_graphicsContext.GetHeight() / 2;
  CGUIImage* image = new CGUIImage(0, 0, 0, 0, w, h, m_ImageName);
  image->SetKeepAspectRatio(true);
  image->AllocResources();
  int x = (g_graphicsContext.GetWidth() - image->GetRenderWidth()) / 2;
  int y = (g_graphicsContext.GetHeight() - image->GetRenderHeight()) / 2;
  image->SetPosition(x, y);

  // Store the old gamma ramp
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldRamp);
  float fade = 0.5f;
  for (int i = 0; i < 256; i++)
  {
    newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.green[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.blue[i] = (int)((float)oldRamp.red[i] * fade);
  }
  g_graphicsContext.Get3DDevice()->SetGammaRamp(D3DSGR_IMMEDIATE, &newRamp);

  //render splash image
  image->Render();
  image->FreeResources();
  delete image;
  //show it on screen
  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();

  //fade in and wait untill the thread is stopped
  while (!m_bStop)
  {
    if (fade <= 1.f)
    {
      Sleep(1);
      for (int i = 0; i < 256; i++)
      {
        newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
        newRamp.green[i] = (int)((float)oldRamp.green[i] * fade);
        newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fade);
      }
      g_graphicsContext.Lock();
      g_graphicsContext.Get3DDevice()->SetGammaRamp(D3DSGR_IMMEDIATE, &newRamp);
      g_graphicsContext.Unlock();
      fade += 0.01f;
    }
    else
    {
      Sleep(10);
    }
  }

  g_graphicsContext.Lock();
  // fade out
  for (float fadeout = fade - 0.01f; fadeout >= 0.f; fadeout -= 0.01f)
  {
    for (int i = 0; i < 256; i++)
    {
      newRamp.red[i] = (int)((float)oldRamp.red[i] * fadeout);
      newRamp.green[i] = (int)((float)oldRamp.green[i] * fadeout);
      newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fadeout);
    }
    Sleep(1);
    g_graphicsContext.Get3DDevice()->SetGammaRamp(D3DSGR_IMMEDIATE, &newRamp);
  }
  //restore original gamma ramp
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  g_graphicsContext.Get3DDevice()->SetGammaRamp(0, &oldRamp);
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();
}

bool CSplash::Start()
{
  if (m_ImageName.IsEmpty() || !CFile::Exists(m_ImageName))
  {
    CLog::Log(LOGDEBUG, "Splash image %s not found", m_ImageName.c_str());
    return false;
  }
  Create();
  return true;
}

void CSplash::Stop()
{
  StopThread();
}

bool CSplash::IsRunning()
{
  return (m_ThreadHandle != NULL);
}
