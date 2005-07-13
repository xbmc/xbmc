#include "../stdafx.h"
#include "ScreenSaverFactory.h"
#include "../cores/DllLoader/dll.h"
#include "../util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
  DllLoader* pDLL = new DllLoader(strScr.c_str(), true);
  if ( !pDLL->Parse() )
  {
    // failed,
    delete pDLL;
    return NULL;
  }
  pDLL->ResolveImports();

  // get handle to the get_module() function from the screensaver
  void (__cdecl* pGetModule)(struct ScreenSaver*);
  struct ScreenSaver* pScr = (struct ScreenSaver*)malloc(sizeof(struct ScreenSaver));
  void* pProc;
  pDLL->ResolveExport("get_module", &pProc);
  if (!pProc)
  {
    // get_module() not found in screensaver
    delete pDLL;
    return NULL;
  }
  // call get_module() to get the ScreenSaver struct from the screensaver
  pGetModule = (void (__cdecl*)(struct ScreenSaver*))pProc;
  pGetModule(pScr);

  // and pass it to a new instance of CScreenSaver() which will handle the screensaver
  return new CScreenSaver(pScr, pDLL, strName);
}
