#ifndef _IFFDSHOWDEC_H_
#define _IFFDSHOWDEC_H_

struct IFilterGraph;
struct Tpreset;
class Tpresets;
struct IMediaSample;
struct TfilterIDFF;
struct TfilterIDFFs;
class TffdshowPageDec;
class TinputPin;
struct Tstrptrs;
template<class tchar> DECLARE_INTERFACE_(IffdshowDecT,IUnknown)
{
 STDMETHOD_(int,getVersion2)(void) PURE;
 STDMETHOD (saveKeysSettings)(void) PURE;
 STDMETHOD (loadKeysSettings)(void) PURE;
 STDMETHOD (getGraph)(IFilterGraph* *graphPtr) PURE;
 STDMETHOD (seek)(int seconds) PURE;
 STDMETHOD (tell)(int*seconds) PURE;
 STDMETHOD (stop)(void) PURE;
 STDMETHOD (run)(void) PURE;
 STDMETHOD_(int,getState2)(void) PURE;
 STDMETHOD (getDuration)(int*seconds) PURE;
 STDMETHOD_(int,getCurTime2)(void) PURE;
 STDMETHOD (initKeys)(void) PURE;
 STDMETHOD_(int,getKeyParamCount2)(void) PURE;
 STDMETHOD (getKeyParamDescr)(unsigned int i,const tchar **descr) PURE;
 STDMETHOD_(int,getKeyParamKey2)(unsigned int i) PURE;
 STDMETHOD (setKeyParamKey)(unsigned int i,int key) PURE;
 STDMETHOD (getNumPresets)(unsigned int *value) PURE;
 STDMETHOD (initPresets)(void) PURE;
 STDMETHOD (getPresetName)(unsigned int i,tchar *buf,size_t len) PURE;
 STDMETHOD (getActivePresetName)(tchar *buf,size_t len) PURE;
 STDMETHOD (setActivePreset)(const tchar *name,int create) PURE;
 STDMETHOD (saveActivePreset)(const tchar *name) PURE;
 STDMETHOD (saveActivePresetToFile)(const tchar *flnm) PURE;
 STDMETHOD (loadActivePresetFromFile)(const tchar *flnm) PURE;
 STDMETHOD (removePreset)(const tchar *name) PURE;
 STDMETHOD (getPresets)(Tpresets *presets2) PURE;
 STDMETHOD (setPresets)(const Tpresets *presets2) PURE;
 STDMETHOD (savePresets)(void) PURE;
 STDMETHOD (getPresetPtr)(Tpreset**preset) PURE;
 STDMETHOD (setPresetPtr)(Tpreset *preset) PURE;
 STDMETHOD (renameActivePreset)(const tchar *newName) PURE;
 STDMETHOD (isDefaultPreset)(const tchar *presetName) PURE;
 STDMETHOD (createTempPreset)(const tchar *presetName) PURE;
 STDMETHOD_(int,getMinOrder2)(void) PURE;
 STDMETHOD_(int,getMaxOrder2)(void) PURE;
 STDMETHOD (resetFilter)(unsigned int filterID) PURE;
 STDMETHOD (resetFilterEx)(unsigned int filterID,unsigned int filterPageId) PURE;
 STDMETHOD (getFilterTip)(unsigned int filterID,tchar *buf,size_t buflen) PURE;
 STDMETHOD (getFilterTipEx)(unsigned int filterID,unsigned int filterPageId,tchar *buf,size_t buflen) PURE;
 STDMETHOD (filterHasReset)(unsigned int filterID) PURE;
 STDMETHOD (filterHasResetEx)(unsigned int filterID,unsigned int filterPageId) PURE;
 STDMETHOD (getPresetsPtr)(Tpresets* *presetsPtr) PURE;
 STDMETHOD (newSample)(IMediaSample* *samplePtr) PURE;
 STDMETHOD (deliverSample_unused)(IMediaSample *sample) PURE;
 STDMETHOD_(const TfilterIDFF*,getFilterIDFF_notimpl)(void) PURE;
 STDMETHOD (resetOrder)(void) PURE;
 STDMETHOD (resetKeys)(void) PURE;
 STDMETHOD (putStringParams)(const tchar *params,tchar sep,int loaddef) PURE;
 STDMETHOD_(const TfilterIDFF*,getNextFilterIDFF)(void) PURE;
 STDMETHOD (cyclePresets)(int step) PURE;
 STDMETHOD (exportKeysToGML)(const tchar *flnm) PURE;
 STDMETHOD (getShortDescription)(tchar *buf,int buflen) PURE;
 STDMETHOD_(const tchar*,getActivePresetName2)(void) PURE;
 STDMETHOD (createPresetPages)(const tchar *presetname,TffdshowPageDec *pages) PURE;
 STDMETHOD (getEncoderInfo)(tchar *buf,size_t buflen) PURE;
 STDMETHOD_(const tchar*,getDecoderName)(void) PURE;
 STDMETHOD (getFilterIDFFs)(const tchar *presetname,const TfilterIDFFs* *filters) PURE;
 STDMETHOD (initRemote)(void) PURE;
 STDMETHOD (saveRemoteSettings)(void) PURE;
 STDMETHOD (loadRemoteSettings)(void) PURE;
 STDMETHOD (setFilterOrder)(unsigned int filterID,unsigned int newOrder) PURE;
 STDMETHOD_(unsigned int,getPresetAutoloadItemsCount2)(void) PURE;
 STDMETHOD (getPresetAutoloadItemInfo)(unsigned int index,const tchar* *name,const tchar* *hint,int *allowWildcard,int *is,int *isVal,tchar *val,size_t vallen,int *isList,int *isHelp) PURE;
 STDMETHOD (setPresetAutoloadItem)(unsigned int index,int is,const tchar *val) PURE;
 STDMETHOD_(const tchar*,getPresetAutoloadItemList)(unsigned int paramIndex,unsigned int listIndex) PURE;
 STDMETHOD_(const tchar**,getSupportedFOURCCs)(void) PURE;
 STDMETHOD_(const Tstrptrs*,getCodecsList)(void) PURE;
 STDMETHOD (queryFilterInterface)(const IID &iid,void **ptr) PURE;
 STDMETHOD (setOnNewFiltersMsg)(HWND wnd,unsigned int msg) PURE;
 STDMETHOD (sendOnNewFiltersMsg)(void) PURE;
 STDMETHOD (setKeyParamKeyCheck)(unsigned int i,int key,int *prev,const tchar* *prevDescr) PURE;
 STDMETHOD_(int,getInputBitrate2)(void) PURE;
 STDMETHOD (getPresetAutoloadItemHelp)(unsigned int index,const tchar* *helpPtr) PURE;
 STDMETHOD_(TinputPin*, getInputPin)(void) PURE;
 STDMETHOD_(CTransformOutputPin*, getOutputPin)(void) PURE;
};

struct IffdshowDecA :IffdshowDecT<char> {};
struct IffdshowDecW :IffdshowDecT<wchar_t> {};

#ifndef DEFINE_TGUID
 #define DEFINE_TGUID(IID,I, l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(IID##_##I,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif
// {00F99065-70D5-4bcc-9D88-3801F3E3881B}
DEFINE_TGUID(IID,IffdshowDecA , 0x00f99065, 0x70d5, 0x4bcc, 0x9d, 0x88, 0x38, 0x01, 0xf3, 0xe3, 0x88, 0x1b)
// {10F99065-70D5-4bcc-9D88-3801F3E3881B}
DEFINE_TGUID(IID,IffdshowDecW , 0x10f99065, 0x70d5, 0x4bcc, 0x9d, 0x88, 0x38, 0x01, 0xf3, 0xe3, 0x88, 0x1b)

#endif
