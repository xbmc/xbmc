// Screensaver.h: interface for the CScreensaver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../cores/DllLoader/dll.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SCR_INFO 
{
	int	dummy;
};

struct ScreenSaver
{
public:
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver);
	void (__cdecl* Start) ();
	void (__cdecl* Render) ();
	void (__cdecl* Stop) ();
	void (__cdecl* GetInfo)(SCR_INFO *info);
} ;

#ifdef __cplusplus
};
#endif

class CScreenSaver
{
public:
	CScreenSaver(struct ScreenSaver* pScr, DllLoader* pLoader, const CStdString& strScreenSaverName);
	 ~CScreenSaver();

	// Things that MUST be supplied by the child classes
	void Create();
	void Start();
	void Render();
	void Stop();
	void GetInfo(SCR_INFO *info);

protected:
	auto_ptr<struct ScreenSaver> m_pScr;
	auto_ptr<DllLoader> m_pLoader;
	CStdString m_strScreenSaverName;
};


#endif // !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
