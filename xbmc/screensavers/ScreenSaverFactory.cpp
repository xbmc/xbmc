#include "stdafx.h"
#include "ScreenSaverFactory.h"
#include "../Util.h"


CScreenSaverFactory::CScreenSaverFactory()
{}
CScreenSaverFactory::~CScreenSaverFactory()
{}

extern "C" void __declspec(dllexport) get_module(struct ScreenSaver* pScr);

CScreenSaver* CScreenSaverFactory::LoadScreenSaver(const CStdString& strScr) const
{
  // strip of the path & extension to get the name of the visualisation
  CStdString strName = CUtil::GetFileName(strScr);
  strName = strName.Left(strName.size() - 4);

  // load visualisation
  DllScreensaver* pDll = new DllScreensaver;
  pDll->SetFile(strScr);
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct ScreenSaver* pScr = (struct ScreenSaver*)malloc(sizeof(struct ScreenSaver));
  ZeroMemory(pScr, sizeof(struct ScreenSaver));
  pDll->GetModule(pScr);

  // and pass it to a new instance of CScreenSaver() which will handle the screensaver
  return new CScreenSaver(pScr, pDll, strName);
}
