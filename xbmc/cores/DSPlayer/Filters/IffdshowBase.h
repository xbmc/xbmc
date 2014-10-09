#ifndef _IFFDSHOWBASE_H_
#define _IFFDSHOWBASE_H_

//for doxygen
#ifndef DECLARE_INTERFACE_
#define DECLARE_INTERFACE_(x,y) struct x: public y
#define STDMETHODCALLTYPE __stdcall
#define STDMETHOD(m) virtual HRESULT STDMETHODCALLTYPE m
#define PURE =0
#define STDMETHODIMP HRESULT STDMETHODCALLTYPE
#define DEFINE_GUID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) GUID_EXT const GUID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#endif

class Ttranslate;
struct Tconfig;
struct TglobalSettingsBase;
struct Tlibmplayer;
struct Tlibavcodec;
struct IFilterGraph;
struct TffdshowParamInfo;
struct IPin;
template<class tchar> DECLARE_INTERFACE_(IffdshowBaseT,IUnknown)
{
 STDMETHOD_(int,getVersion2)(void) PURE;
 STDMETHOD (getParam)(unsigned int paramID, int* value) PURE;
 STDMETHOD_(int,getParam2)(unsigned int paramID) PURE;
 STDMETHOD (putParam)(unsigned int paramID, int  value) PURE;
 STDMETHOD (invParam)(unsigned int paramID) PURE;
 STDMETHOD (getParamStr)(unsigned int paramID,tchar *buf,size_t buflen) PURE;
 STDMETHOD_(const tchar*,getParamStr2)(unsigned int paramID) PURE; //returns const pointer to string, NULL if fail
 STDMETHOD (putParamStr)(unsigned int paramID,const tchar *buf) PURE;
 STDMETHOD (getParamName)(unsigned int i,tchar *buf,size_t len) PURE;
 STDMETHOD (notifyParamsChanged)(void) PURE;
 STDMETHOD (setOnChangeMsg)(HWND wnd,unsigned int msg) PURE;
 STDMETHOD (setOnFrameMsg)(HWND wnd,unsigned int msg) PURE;
 STDMETHOD (getGlobalSettings)(TglobalSettingsBase* *globalSettingsPtr) PURE;
 STDMETHOD (saveGlobalSettings)(void) PURE;
 STDMETHOD (loadGlobalSettings)(void) PURE;
 STDMETHOD (saveDialogSettings)(void) PURE;
 STDMETHOD (loadDialogSettings)(void) PURE;
 STDMETHOD (getConfig)(const Tconfig* *configPtr) PURE;
 STDMETHOD (getInstance)(HINSTANCE *hi) PURE;
 STDMETHOD_(HINSTANCE,getInstance2)(void) PURE;
 STDMETHOD (getPostproc)(Tlibmplayer* *postprocPtr) PURE;
 STDMETHOD (getTranslator)(Ttranslate* *trans) PURE;
 STDMETHOD (initDialog)(void) PURE;
 STDMETHOD (showCfgDlg)(HWND owner) PURE;
 STDMETHOD_(int,getCpuUsage2)(void) PURE;
 STDMETHOD_(int,getCpuUsageForPP)(void) PURE;
 STDMETHOD (cpuSupportsMMX)(void) PURE;
 STDMETHOD (cpuSupportsMMXEXT)(void) PURE;
 STDMETHOD (cpuSupportsSSE)(void) PURE;
 STDMETHOD (cpuSupportsSSE2)(void) PURE;
 STDMETHOD (cpuSupportsSSE3)(void) PURE;
 STDMETHOD (cpuSupportsSSSE3)(void) PURE;
 STDMETHOD (cpuSupports3DNOW)(void) PURE;
 STDMETHOD (cpuSupports3DNOWEXT)(void) PURE;
 STDMETHOD (dbgInit)(void) PURE;
 STDMETHOD (dbgError)(const tchar *fmt,...) PURE;
 STDMETHOD (dbgWrite)(const tchar *fmt,...) PURE;
 STDMETHOD (dbgDone)(void) PURE;
 STDMETHOD (showTrayIcon)(void) PURE;
 STDMETHOD (hideTrayIcon)(void) PURE;
 STDMETHOD_(const tchar*,getExeflnm)(void) PURE;
 STDMETHOD (getLibavcodec)(Tlibavcodec* *libavcodecPtr) PURE;
 STDMETHOD_(const tchar*,getSourceName)(void) PURE;
 STDMETHOD (getGraph)(IFilterGraph* *graphPtr) PURE;
 STDMETHOD (seek)(int seconds) PURE;
 STDMETHOD (tell)(int*seconds) PURE;
 STDMETHOD (stop)(void) PURE;
 STDMETHOD (run)(void) PURE;
 STDMETHOD_(int,getState2)(void) PURE;
 STDMETHOD_(int,getCurTime2)(void) PURE;
 STDMETHOD (getParamStr3)(unsigned int paramID,const tchar* *bufPtr) PURE;
 STDMETHOD (savePresetMem)(void *buf,size_t len) PURE; //if len=0, then buf should point to int variable which will be filled with required buffer length
 STDMETHOD (loadPresetMem)(const void *buf,size_t len) PURE;
 STDMETHOD (getParamName3)(unsigned int i,const tchar* *namePtr) PURE;
 STDMETHOD (getInCodecString)(tchar *buf,size_t buflen) PURE;
 STDMETHOD (getOutCodecString)(tchar *buf,size_t buflen) PURE;
 STDMETHOD (getMerit)(DWORD *merit) PURE;
 STDMETHOD (setMerit)(DWORD  merit) PURE;
 STDMETHOD (lock)(int lockId) PURE;
 STDMETHOD (unlock)(int lockId) PURE;
 STDMETHOD (getParamInfo)(unsigned int i,TffdshowParamInfo *paramPtr) PURE;
 STDMETHOD (exportRegSettings)(int all,const tchar *regflnm,int unicode) PURE;
 STDMETHOD (checkInputConnect)(IPin *pin) PURE;
 STDMETHOD (getParamListItem)(int paramId,int index,const tchar* *ptr) PURE;
 STDMETHOD (abortPlayback)(HRESULT hr) PURE;
 STDMETHOD (notifyParam)(int id,int val) PURE;
 STDMETHOD (notifyParamStr)(int id,const tchar *val) PURE;
 STDMETHOD (doneDialog)(void) PURE;
 STDMETHOD (resetParam)(unsigned int paramID) PURE;
 STDMETHOD_(int,getCurrentCodecId2)(void) PURE;
 STDMETHOD (frameStep)(int diff) PURE;
 STDMETHOD (getInfoItem)(unsigned int index,int *id,const tchar* *name) PURE;
 STDMETHOD (getInfoItemValue)(int id,const tchar* *value,int *wasChange,int *splitline) PURE;
 STDMETHOD (inExplorer)(void) PURE;
 STDMETHOD_(const tchar*,getInfoItemName)(int id) PURE;
 STDMETHOD_(HWND,getCfgDlgHwnd)(void) PURE;
 STDMETHOD_(void,setCfgDlgHwnd)(HWND hwnd) PURE;
 STDMETHOD_(HWND,getTrayHwnd_)(void) PURE;
 STDMETHOD_(void,setTrayHwnd_)(HWND hwnd) PURE;
 STDMETHOD_(const tchar*,getInfoItemShortcut)(int id) PURE;
 STDMETHOD_(int,getInfoShortcutItem)(const tchar *s,int *toklen) PURE;
 STDMETHOD_(DWORD,CPUcount)(void) PURE;
 STDMETHOD_(int,get_trayIconType)(void) PURE;
 STDMETHOD (cpuSupportsSSE41)(void) PURE;
 STDMETHOD (cpuSupportsSSE42)(void) PURE;
 STDMETHOD (cpuSupportsSSE4A)(void) PURE;
 STDMETHOD (cpuSupportsSSE5)(void) PURE;
};

struct IffdshowBaseA :IffdshowBaseT<char> {};
struct IffdshowBaseW :IffdshowBaseT<wchar_t> {};

#ifndef DEFINE_TGUID
 #define DEFINE_TGUID(IID,I, l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(IID##_##I,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif
// {EC5BCCF4-FD62-45ee-B022-3840EAEA77B2}
DEFINE_TGUID(IID,IffdshowBaseA,0xec5bccf4, 0xfd62, 0x45ee, 0xb0, 0x22, 0x38, 0x40, 0xea, 0xea, 0x77, 0xb2)
// {FC5BCCF4-FD62-45ee-B022-3840EAEA77B2}
DEFINE_TGUID(IID,IffdshowBaseW,0xfc5bccf4, 0xfd62, 0x45ee, 0xb0, 0x22, 0x38, 0x40, 0xea, 0xea, 0x77, 0xb2)

enum
{
 IDFF_lockOSDuser   =1,
 IDFF_lockDScaler     ,
 IDFF_lockSublangs    ,
 IDFF_lockPresetPtr   ,
 IDFF_lockInfo        ,
 LOCKS_COUNT
};

#endif
