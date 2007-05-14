#ifdef WITH_LINKS_BROWSER

#pragma once

#include "DynamicDll.h"
#define XBMC_LINKS_DLL
#define XBOX_USE_FREETYPE
#include "lib/linksboks/linksboks.h"

// dll defines

class DllLinksBoksInterface
{
  virtual int LinksBoks_InitCore(unsigned char *homedir, LinksBoksOption *options[], LinksBoksProtocol *protocols[])=0;
  virtual LinksBoksWindow *LinksBoks_CreateWindow(LPDIRECT3DDEVICE8 pd3dDevice, LinksBoksViewPort viewport)=0;
  virtual int LinksBoks_FrameMove(void)=0;
  virtual VOID LinksBoks_EmptyCaches(void)=0;
  virtual void LinksBoks_Terminate(BOOL bFreeXBESections)=0;
  virtual VOID LinksBoks_FreezeCore(void)=0;
  virtual VOID LinksBoks_UnfreezeCore(void)=0;
  virtual int LinksBoks_RegisterNewTimer(long t, void (*func)(void *), void *data)=0;
  virtual VOID LinksBoks_SetExecFunction(int (*exec_function)(LinksBoksWindow *pLB, unsigned char *cmdline, unsigned char *filepath, int fg))=0;
  virtual VOID LinksBoks_SetExtFontCallbackFunction(LinksBoksExtFont *(*extfont_function)(unsigned char *fontname, int fonttype))=0;
  virtual VOID LinksBoks_SetMessageBoxFunction(BOOL (*msgbox_function)(void *dlg, unsigned char *title, int nblabels, unsigned char *labels[], int nbbuttons, unsigned char *buttons[]))=0;
  virtual VOID LinksBoks_ValidateMessageBox(void *dlg, int choice)=0;
  virtual ILinksBoksURLList *LinksBoks_GetURLList(int type)=0;
  virtual ILinksBoksBookmarksWriter *LinksBoks_GetBookmarksWriter(void)=0;
  virtual BOOL LinksBoks_GetOptionBool(const char *key)=0;
  virtual INT LinksBoks_GetOptionInt(const char *key)=0;
  virtual unsigned char *LinksBoks_GetOptionString(const char *key)=0;
  virtual void LinksBoks_SetOptionBool(const char *key, BOOL value)=0;
  virtual void LinksBoks_SetOptionInt(const char *key, INT value)=0;
  virtual void LinksBoks_SetOptionString(const char *key, unsigned char *value)=0;
};

class DllLinksBoks : public DllDynamic, DllLinksBoksInterface
{
#ifndef _DEBUG
  DECLARE_DLL_WRAPPER(DllLinksBoks, Q:\\system\\LinksBrowser\\LinksBoks.dll)
#else
  DECLARE_DLL_WRAPPER(DllLinksBoks, Q:\\system\\LinksBrowser\\LinksBoks_dbg.dll)
  LOAD_SYMBOLS()
#endif
  DEFINE_METHOD3(int, LinksBoks_InitCore, (unsigned char *p1, LinksBoksOption *p2[], LinksBoksProtocol *p3[]))
  DEFINE_METHOD2(LinksBoksWindow *, LinksBoks_CreateWindow, (LPDIRECT3DDEVICE8 p1, LinksBoksViewPort p2))
  DEFINE_METHOD0(int, LinksBoks_FrameMove)
  DEFINE_METHOD0(VOID, LinksBoks_EmptyCaches)
  DEFINE_METHOD1(void, LinksBoks_Terminate, (BOOL p1))
  DEFINE_METHOD0(VOID, LinksBoks_FreezeCore)
  DEFINE_METHOD0(VOID, LinksBoks_UnfreezeCore)
  DEFINE_METHOD3(int, LinksBoks_RegisterNewTimer, (long p1, void (*p2)(void *), void *p3))
  DEFINE_METHOD1(VOID, LinksBoks_SetExecFunction, (int (*p1)(LinksBoksWindow *, unsigned char *, unsigned char *, int)))
  DEFINE_METHOD1(VOID, LinksBoks_SetExtFontCallbackFunction, (LinksBoksExtFont *(*p1)(unsigned char *, int)))
  DEFINE_METHOD1(VOID, LinksBoks_SetMessageBoxFunction, (BOOL (*p1)(void *, unsigned char *, int, unsigned char **, int, unsigned char **)))
  DEFINE_METHOD2(VOID, LinksBoks_ValidateMessageBox, (void *p1, int p2))
  DEFINE_METHOD1(ILinksBoksURLList *, LinksBoks_GetURLList, (int p1))
  DEFINE_METHOD0(ILinksBoksBookmarksWriter *, LinksBoks_GetBookmarksWriter)
  DEFINE_METHOD1(BOOL, LinksBoks_GetOptionBool, (const char *p1))
  DEFINE_METHOD1(INT, LinksBoks_GetOptionInt, (const char *p1))
  DEFINE_METHOD1(unsigned char *, LinksBoks_GetOptionString, (const char *p1))
  DEFINE_METHOD2(void, LinksBoks_SetOptionBool, (const char *p1, BOOL p2))
  DEFINE_METHOD2(void, LinksBoks_SetOptionInt, (const char *p1, INT p2))
  DEFINE_METHOD2(void, LinksBoks_SetOptionString, (const char *p1, unsigned char *p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(LinksBoks_InitCore)
    RESOLVE_METHOD(LinksBoks_CreateWindow)
    RESOLVE_METHOD(LinksBoks_FrameMove)
    RESOLVE_METHOD(LinksBoks_EmptyCaches)
    RESOLVE_METHOD(LinksBoks_Terminate)
    RESOLVE_METHOD(LinksBoks_FreezeCore)
    RESOLVE_METHOD(LinksBoks_UnfreezeCore)
    RESOLVE_METHOD(LinksBoks_RegisterNewTimer)
    RESOLVE_METHOD(LinksBoks_SetExecFunction)
    RESOLVE_METHOD(LinksBoks_SetExtFontCallbackFunction)
    RESOLVE_METHOD(LinksBoks_SetMessageBoxFunction)
    RESOLVE_METHOD(LinksBoks_ValidateMessageBox)
    RESOLVE_METHOD(LinksBoks_GetURLList)
    RESOLVE_METHOD(LinksBoks_GetBookmarksWriter)
    RESOLVE_METHOD(LinksBoks_GetOptionBool)
    RESOLVE_METHOD(LinksBoks_GetOptionInt)
    RESOLVE_METHOD(LinksBoks_GetOptionString)
    RESOLVE_METHOD(LinksBoks_SetOptionBool)
    RESOLVE_METHOD(LinksBoks_SetOptionInt)
    RESOLVE_METHOD(LinksBoks_SetOptionString)
  END_METHOD_RESOLVE()
};


class CLinksBoksManager
{
public:
  CLinksBoksManager(void);
  virtual ~CLinksBoksManager(void);

  bool LoadDLL();
  void UnloadDLL();
  
  bool Initialize();
  void FrameMove();
  void Terminate();

  bool Freeze();
  bool UnFreeze();

  bool CreateBrowserWindow(int width, int height);
  ILinksBoksWindow *GetBrowserWindow();
  bool CloseBrowserWindow();

  void EmptyCaches();

  void RegisterFont(LinksBoksExtFont *xfont);
  void UnregisterFont(LinksBoksExtFont *xfont);
  LinksBoksExtFont *GetFont(const CStdString& strFontName);

  bool RefreshSettings(bool bRedraw);

  void ValidateMsgBox(void *dlg, int choice);

  void SetExpertMode(BOOL mode);
  BOOL GetExpertMode();

  CStdString GetCurrentTitle();
  CStdString GetCurrentURL();
  CStdString GetCurrentStatus();
  int GetCurrentState();

  ILinksBoksURLList *GetURLList(int type);
  ILinksBoksBookmarksWriter *GetBookmarksWriter();

private:
  ILinksBoksWindow *m_window;
  LinksBoksViewPort m_viewport;
  bool m_IsLoaded;
  bool m_Initialized;
  bool m_Sleeping;
  DllLinksBoks m_dll;

  vector<LinksBoksExtFont*> m_vecFonts;

public:
  BOOL isLoaded() { return m_IsLoaded && m_Initialized; }
  BOOL isRunning() { return m_IsLoaded && m_Initialized && !m_Sleeping; }
  BOOL isFrozen() { return m_IsLoaded && m_Initialized && m_Sleeping; }

};

extern CLinksBoksManager g_browserManager;

#endif