// Screensaver.h: interface for the CScreensaver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DllScreensaver.h"

#ifdef __cplusplus
extern "C"
{
}
#endif

class CScreenSaver
{
public:
  CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const CStdString& strScreenSaverName);
  ~CScreenSaver();

  // Things that MUST be supplied by the child classes
  void Create();
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO *info);

protected:
  auto_ptr<struct ScreenSaver> m_pScr;
  auto_ptr<DllScreensaver> m_pDll;
  CStdString m_strScreenSaverName;
};


#endif // !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
