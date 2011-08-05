#ifndef _IFFDECODER_H_
#define _IFFDECODER_H_

#ifdef __cplusplus
extern "C" {
#endif

// {00F99063-70D5-4bcc-9D88-3801F3E3881B}
DEFINE_GUID(IID_IffDecoder , 0x00f99063, 0x70d5, 0x4bcc, 0x9d, 0x88, 0x38, 0x01, 0xf3, 0xe3, 0x88, 0x1b);
// {5DD3A966-7365-46d0-B79F-D4973BD88E8D}
DEFINE_GUID(IID_IffDecoder2, 0x5dd3a966, 0x7365, 0x46d0, 0xb7, 0x9f, 0xd4, 0x97, 0x3b, 0xd8, 0x8e, 0x8d);

struct Tpreset;
struct TglobalSettingsDecVideo;
class Tpresets;
class TvideoCodecDec;
struct TffPict;
class TimgFilters;
class Ttranslate;
struct IFilterGraph;
struct Tconfig;
struct Tlibmplayer;
DECLARE_INTERFACE_(IffDecoder, IUnknown)
{
 STDMETHOD (compat_getParam)(unsigned int paramID, int* value) PURE;
 STDMETHOD (compat_getParam2)(unsigned int paramID) PURE;
 STDMETHOD (compat_putParam)(unsigned int paramID, int  value) PURE;
 STDMETHOD (compat_getNumPresets)(unsigned int *value) PURE;
 STDMETHOD (compat_getPresetName)(unsigned int i,char *buf,unsigned int len) PURE;
 STDMETHOD (compat_getActivePresetName)(char *buf,unsigned int len) PURE;
 STDMETHOD (compat_setActivePreset)(const char *name,int create) PURE;
 STDMETHOD (compat_getAVIdimensions)(unsigned int *x,unsigned int *y) PURE;
 STDMETHOD (compat_getAVIfps)(unsigned int *fps1000) PURE;
 STDMETHOD (compat_saveActivePreset)(const char *name) PURE;
 STDMETHOD (compat_saveActivePresetToFile)(const char *flnm) PURE;
 STDMETHOD (compat_loadActivePresetFromFile)(const char *flnm) PURE;
 STDMETHOD (compat_removePreset)(const char *name) PURE;
 STDMETHOD (compat_notifyParamsChanged)(void) PURE;
 STDMETHOD (compat_getAVcodecVersion)(char *buf,unsigned int len) PURE;
 STDMETHOD (compat_getPPmode)(unsigned int *ppmode) PURE;
 STDMETHOD (compat_getRealCrop)(unsigned int *left,unsigned int *top,unsigned int *right,unsigned int *bottom) PURE;
 STDMETHOD (compat_getMinOrder2)(void) PURE;
 STDMETHOD (compat_getMaxOrder2)(void) PURE;
 STDMETHOD (compat_saveGlobalSettings)(void) PURE;
 STDMETHOD (compat_loadGlobalSettings)(void) PURE;
 STDMETHOD (compat_saveDialogSettings)(void) PURE;
 STDMETHOD (compat_loadDialogSettings)(void) PURE;
 STDMETHOD (compat_getPresets)(Tpresets *presets2) PURE;
 STDMETHOD (compat_setPresets)(const Tpresets *presets2) PURE;
 STDMETHOD (compat_savePresets)(void) PURE;
 STDMETHOD (compat_getPresetPtr)(Tpreset**preset) PURE;
 STDMETHOD (compat_setPresetPtr)(Tpreset *preset) PURE;
 STDMETHOD (compat_renameActivePreset)(const char *newName) PURE;
 STDMETHOD (compat_setOnChangeMsg)(HWND wnd,unsigned int msg) PURE;
 STDMETHOD (compat_setOnFrameMsg)(HWND wnd,unsigned int msg) PURE;
 STDMETHOD (compat_isDefaultPreset)(const char *presetName) PURE;
 STDMETHOD (compat_showCfgDlg)(HWND owner) PURE;
 STDMETHOD (compat_getXvidVersion)(char *buf,unsigned int len) PURE;
 STDMETHOD (compat_getMovieSource)(const TvideoCodecDec* *moviePtr) PURE;
 STDMETHOD (compat_getOutputDimensions)(unsigned int *x,unsigned int *y) PURE;
 STDMETHOD (compat_getCpuUsage2)(void) PURE;
 STDMETHOD (compat_getOutputFourcc)(char *buf,unsigned int len) PURE;
 STDMETHOD (compat_getInputBitrate2)(void) PURE;
 STDMETHOD (compat_getHistogram)(unsigned int dst[256]) PURE;
 STDMETHOD (compat_setFilterOrder)(unsigned int filterID,unsigned int newOrder) PURE;
 STDMETHOD (compat_buildHistogram)(const TffPict *pict,int full) PURE;
 STDMETHOD (compat_cpuSupportsMMX)(void) PURE;
 STDMETHOD (compat_cpuSupportsMMXEXT)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE2)(void) PURE;
 STDMETHOD (compat_cpuSupports3DNOW)(void) PURE;
 STDMETHOD (compat_cpuSupports3DNOWEXT)(void) PURE;
 STDMETHOD (compat_getAVIfps1000_2)(void) PURE;
 STDMETHOD (compat_getParamStr)(unsigned int paramID,char *buf,unsigned int buflen) PURE;
 STDMETHOD (compat_putParamStr)(unsigned int paramID,const char *buf) PURE;
 STDMETHOD (compat_invParam)(unsigned int paramID) PURE;
 STDMETHOD (compat_getInstance)(HINSTANCE *hi) PURE;
 STDMETHOD (compat_saveKeysSettings)(void) PURE;
 STDMETHOD (compat_loadKeysSettings)(void) PURE;
 STDMETHOD (compat_seek)(int seconds) PURE;
 STDMETHOD (compat_tell)(int*seconds) PURE;
 STDMETHOD (compat_getDuration)(int*seconds) PURE;
 STDMETHOD (compat_getKeyParamCount2)(void) PURE;
 STDMETHOD (compat_getKeyParamDescr)(unsigned int i,const char **descr) PURE;
 STDMETHOD (compat_getKeyParamKey2)(unsigned int i) PURE;
 STDMETHOD (compat_setKeyParamKey)(unsigned int i,int key) PURE;
 STDMETHOD (compat_getImgFilters)(TimgFilters* *imgFiltersPtr) PURE;
 STDMETHOD (compat_getQuant)(int* *quantPtr) PURE;
 STDMETHOD (compat_calcNewSize)(unsigned int inDx,unsigned int inDy,unsigned int *outDx,unsigned int *outDy) PURE;
 STDMETHOD (compat_grabNow)(void) PURE;
 STDMETHOD (compat_getOverlayControlCapability)(int idff) PURE; //S_OK - can be set, S_FALSE - not supported
 STDMETHOD (compat_getParamName)(unsigned int i,char *buf,unsigned int len) PURE;
 STDMETHOD (compat_getTranslator)(Ttranslate* *trans) PURE;
 STDMETHOD (compat_getIffDecoderVersion2)(void) PURE;
 STDMETHOD (compat_lock)(int lockId) PURE;
 STDMETHOD (compat_unlock)(int lockId) PURE;
 STDMETHOD (compat_getInstance2)(void) PURE;
 STDMETHOD (compat_getGraph)(IFilterGraph* *graphPtr) PURE;
 STDMETHOD (compat_getConfig)(Tconfig* *configPtr) PURE;
 STDMETHOD (compat_initDialog)(void) PURE;
 STDMETHOD (compat_initPresets)(void) PURE;
 STDMETHOD (compat_calcMeanQuant)(float *quant) PURE;
 STDMETHOD (compat_initKeys)(void) PURE;
 STDMETHOD (compat_savePresetMem)(void *buf,unsigned int len) PURE; //if len=0, then buf should point to int variable which will be filled with required buffer length
 STDMETHOD (compat_loadPresetMem)(const void *buf,unsigned int len) PURE;
 STDMETHOD (compat_getGlobalSettings)(TglobalSettingsDecVideo* *globalSettingsPtr) PURE;
 STDMETHOD (compat_createTempPreset)(const char *presetName) PURE;
 STDMETHOD (compat_getParamStr2)(unsigned int paramID) PURE; //returns const pointer to string, NULL if fail
 STDMETHOD (compat_findAutoSubflnm2)(void) PURE;
 STDMETHOD (compat_getCurrentFrameTime)(unsigned int *sec) PURE;
 STDMETHOD (compat_getFrameTime)(unsigned int framenum,unsigned int *sec) PURE;
 STDMETHOD (compat_getCurTime2)(void) PURE;
 STDMETHOD (compat_getPostproc)(Tlibmplayer* *postprocPtr) PURE;
 STDMETHOD (compat_stop)(void) PURE;
 STDMETHOD (compat_run)(void) PURE;
 STDMETHOD (compat_getState2)(void) PURE;
 STDMETHOD (compat_resetFilter)(unsigned int filterID) PURE;
 STDMETHOD (compat_resetFilterEx)(unsigned int filterID,unsigned int filterPageId) PURE;
 STDMETHOD (compat_getFilterTip)(unsigned int filterID,char *buf,unsigned int buflen) PURE;
 STDMETHOD (compat_getFilterTipEx)(unsigned int filterID,unsigned int filterPageId,char *buf,unsigned int buflen) PURE;
 STDMETHOD (compat_filterHasReset)(unsigned int filterID) PURE;
 STDMETHOD (compat_filterHasResetEx)(unsigned int filterID,unsigned int filterPageId) PURE;
 STDMETHOD (compat_shortOSDmessage)(const char *msg,unsigned int duration) PURE; //duration is in frames
 STDMETHOD (compat_shortOSDmessageAbsolute)(const char *msg,unsigned int duration,unsigned posX,unsigned int posY) PURE; //duration is in frames
 STDMETHOD (compat_cleanShortOSDmessages)(void) PURE;
 STDMETHOD (compat_setImgFilters)(TimgFilters *imgFiltersPtr) PURE;
 STDMETHOD (compat_registerSelectedMediaTypes)(void) PURE;
 STDMETHOD (compat_getFrameTimes)(int64_t *start,int64_t *stop) PURE;
 STDMETHOD (compat_getSubtitleTimes)(int64_t *start,int64_t *stop) PURE;
 STDMETHOD (compat_resetSubtitleTimes)(void) PURE;
 STDMETHOD (compat_setFrameTimes)(int64_t start,int64_t stop) PURE;
 STDMETHOD (compat_cpuSupportsSSE41)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE42)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE4A)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE5)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSE3)(void) PURE;
 STDMETHOD (compat_cpuSupportsSSSE3)(void) PURE;
 STDMETHOD (compat_getIffDecoder2Version)(void) PURE;
 STDMETHOD (compat_getParamStrW)(unsigned int paramID,wchar_t *buf,unsigned int buflen) PURE;
 STDMETHOD (compat_putParamStrW)(unsigned int paramID,const wchar_t *buf) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
