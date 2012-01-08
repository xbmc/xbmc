#ifndef _IFFDSHOWDECVIDEO_H_
#define _IFFDSHOWDECVIDEO_H_

struct TpresetVideo;
class Tpresets;
class TvideoCodecDec;
struct TffPict;
class TfontManager;
struct IMixerPinConfig2;
struct IcheckSubtitle;
struct IOSDprovider;
struct IhwOverlayControl;
struct ToutputVideoSettings;
struct Trect;

template<class tchar> DECLARE_INTERFACE_(IffdshowDecVideoT,IUnknown)
{
 STDMETHOD_(int,getVersion2)(void) PURE;
 STDMETHOD (getAVIdimensions)(unsigned int *x,unsigned int *y) PURE;
 STDMETHOD (getAVIfps)(unsigned int *fps1000) PURE;
 STDMETHOD (getAVcodecVersion)(tchar *buf,size_t len) PURE;
 STDMETHOD (getPPmode)(unsigned int *ppmode) PURE;
 STDMETHOD (getRealCrop)(unsigned int *left,unsigned int *top,unsigned int *right,unsigned int *bottom) PURE;
 STDMETHOD (getXvidVersion)(tchar *buf,size_t len) PURE;
 STDMETHOD (getMovieSource)(const TvideoCodecDec* *moviePtr) PURE;
 STDMETHOD (getOutputDimensions)(unsigned int *x,unsigned int *y) PURE;
 STDMETHOD (getOutputFourcc)(tchar *buf,size_t len) PURE;
 STDMETHOD_(int,getInputBitrate2)(void) PURE;
 STDMETHOD (getHistogram_)(unsigned int dst[256]) PURE;
 STDMETHOD (setFilterOrder)(unsigned int filterID,unsigned int newOrder) PURE;
 STDMETHOD (buildHistogram_)(const TffPict *pict,int full) PURE;
 STDMETHOD_(int,getAVIfps1000_2)(void) PURE;
 STDMETHOD (getCurrentFrameTime)(unsigned int *sec) PURE;
 STDMETHOD (getImgFilters_)(void* *imgFiltersPtr) PURE;
 STDMETHOD (getQuant)(int* *quantPtr) PURE;
 STDMETHOD (calcNewSize)(unsigned int inDx,unsigned int inDy,unsigned int *outDx,unsigned int *outDy) PURE;
 STDMETHOD (grabNow)(void) PURE;
 STDMETHOD (getOverlayControlCapability)(int idff) PURE; //S_OK - can be set, S_FALSE - not supported
 STDMETHOD (lock)(int lockId) PURE;
 STDMETHOD (unlock)(int lockId) PURE;
 STDMETHOD (calcMeanQuant)(float *quant) PURE;
 STDMETHOD_(int,findAutoSubflnm2)(void) PURE;
 STDMETHOD (getFrameTime)(unsigned int framenum,unsigned int *sec) PURE;
 STDMETHOD (shortOSDmessage)(const tchar *msg,unsigned int duration) PURE; //duration is in frames
 STDMETHOD (cleanShortOSDmessages)(void) PURE;
 STDMETHOD (shortOSDmessageAbsolute)(const tchar *msg,unsigned int duration,unsigned int posX,unsigned int posY) PURE; //duration is in frames
 STDMETHOD (setImgFilters_)(void *imgFiltersPtr) PURE;
 STDMETHOD (registerSelectedMediaTypes)(void) PURE;
 STDMETHOD (getFrameTimes)(int64_t *start,int64_t *stop) PURE;
 STDMETHOD (getSubtitleTimes)(int64_t *start,int64_t *stop) PURE;
 STDMETHOD (resetSubtitleTimes)(void) PURE;
 STDMETHOD (setFrameTimes)(int64_t start,int64_t stop) PURE;
 STDMETHOD_(int,getCodecId)(const BITMAPINFOHEADER *hdr,const GUID *subtype,FOURCC *AVIfourcc) PURE;
 STDMETHOD (getFontManager)(TfontManager* *fontManagerPtr) PURE;
 STDMETHOD_(int,getInIsSync)(void) PURE;
 STDMETHOD (getVideoWindowPos)(int *left,int *top,unsigned int *width,unsigned int *height) PURE;
 STDMETHOD_(unsigned int,getSubtitleLanguagesCount2)(void) PURE;
 STDMETHOD (getSubtitleLanguageDesc)(unsigned int num,const tchar* *descPtr) PURE;
 STDMETHOD (fillSubtitleLanguages)(const tchar **langs) PURE;
 STDMETHOD (getFrameTimeMS)(unsigned int framenum,unsigned int *msec) PURE;
 STDMETHOD (getCurrentFrameTimeMS)(unsigned int *msec) PURE;
 STDMETHOD (frameStep)(int diff) PURE;
 STDMETHOD (textPinConnected_)(unsigned int num) PURE;
 STDMETHOD (cycleSubLanguages)(int diff) PURE;
 STDMETHOD (getLevelsMap)(unsigned int map[256]) PURE;
 STDMETHOD_(const tchar*,findAutoSubflnm3)(void) PURE;
 STDMETHOD (setAverageTimePerFrame)(int64_t *avg,int useDef) PURE;
 STDMETHOD (getLate)(int64_t *latePtr) PURE;
 STDMETHOD (getAverageTimePerFrame)(int64_t *avg) PURE;
 STDMETHOD_(const tchar*,getEmbeddedSubtitleName2_)(unsigned int num) PURE;
 STDMETHOD (putHistogram_)(unsigned int Ihistogram[256]) PURE;
 STDMETHOD_(const tchar*,getCurrentSubFlnm)(void) PURE;
 STDMETHOD (quantsAvailable)(void) PURE;
 STDMETHOD (isNeroAVC_)(void) PURE;
 STDMETHOD (findOverlayControl)(IMixerPinConfig2* *overlayPtr) PURE;
 STDMETHOD (getVideoDestRect)(RECT *r) PURE;
 STDMETHOD_(FOURCC,getMovieFOURCC)(void) PURE;
 STDMETHOD (getRemainingFrameTime)(unsigned int *sec) PURE;
 STDMETHOD (getInputSAR)(unsigned int *a1,unsigned int *a2) PURE;
 STDMETHOD (getInputDAR)(unsigned int *a1,unsigned int *a2) PURE;
 STDMETHOD (getQuantMatrices)(unsigned char intra8[64],unsigned char inter8[64]) PURE;
 STDMETHOD_(const tchar*,findAutoSubflnms)(IcheckSubtitle *checkSubtitle) PURE;
 STDMETHOD (addClosedCaption)(const wchar_t* line) PURE;
 STDMETHOD (hideClosedCaptions)(void) PURE;
 STDMETHOD_(int,getConnectedTextPinCnt)(void) PURE;
 STDMETHOD (getConnectedTextPinInfo)(int i,const tchar* *name,int *id,int *found) PURE;
 STDMETHOD (registerOSDprovider)(IOSDprovider *provider,const char *name) PURE;
 STDMETHOD (unregisterOSDprovider)(IOSDprovider *provider) PURE;
 STDMETHOD (findOverlayControl2)(IhwOverlayControl* *overlayPtr) PURE;
 STDMETHOD_(int,getOSDtime)(void) PURE;
 STDMETHOD_(int,getQueuedCount)(void) PURE;
 STDMETHOD_(__int64,getLate)(void) PURE;
 STDMETHOD_(const char*,get_current_idct)(void) PURE;
 STDMETHOD_(int,get_time_on_ffdshow)(void) PURE;
 STDMETHOD_(int,get_time_on_ffdshow_percent)(void) PURE;
 STDMETHOD_(bool,shouldSkipH264loopFilter)(void) PURE;
 STDMETHOD_(int,get_downstreamID)(void) PURE;
 STDMETHOD_(const char*,getAviSynthInfo)(void) PURE;
 STDMETHOD (lockCSReceive)(void) PURE;
 STDMETHOD (unlockCSReceive)(void) PURE;
 STDMETHOD_(ToutputVideoSettings*,getToutputVideoSettings)(void) PURE;
 STDMETHOD_(int,getBordersBrightness)(void) PURE;
 STDMETHOD (getChaptersList)(void **ppChaptersList) PURE;
 STDMETHOD (get_CurrentTime)(__int64 *time) PURE;
 STDMETHOD_(const Trect*,getDecodedPictdimensions)(void) PURE;
 STDMETHOD_(HANDLE,getGlyphThreadHandle)(void) PURE;
 STDMETHOD_(void*,getRateInfo)(void) PURE;
 STDMETHOD (lock_csCodecs_and_imgFilters)(void) PURE;
 STDMETHOD (unlock_csCodecs_and_imgFilters)(void) PURE;
 STDMETHOD_(void*, get_csReceive_ptr)(void) PURE;
 STDMETHOD_(void*, get_csCodecs_and_imgFilters_ptr)(void) PURE;
 STDMETHOD (reconnectOutput)(const TffPict &newpict) PURE;
};

struct IffdshowDecVideoA :IffdshowDecVideoT<char> {};
struct IffdshowDecVideoW :IffdshowDecVideoT<wchar_t> {};
struct IVMRffdshow9;

#ifndef DEFINE_TGUID
 #define DEFINE_TGUID(IID,I, l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(IID##_##I,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif
// {00F99064-70D5-4bcc-9D88-3801F3E3881B}
DEFINE_TGUID(IID,IffdshowDecVideoA, 0x00f99064, 0x70d5, 0x4bcc, 0x9d, 0x88, 0x38, 0x01, 0xf3, 0xe3, 0x88, 0x1b)
// {00F99064-70D5-4bcc-9D88-3801F3E3881B}
DEFINE_TGUID(IID,IffdshowDecVideoW, 0x10f99064, 0x70d5, 0x4bcc, 0x9d, 0x88, 0x38, 0x01, 0xf3, 0xe3, 0x88, 0x1b)
// {04FE9017-F873-410e-871E-AB91661A4EF7}
DEFINE_GUID(CLSID_FFDSHOW         , 0x04fe9017, 0xf873, 0x410e, 0x87, 0x1e, 0xab, 0x91, 0x66, 0x1a, 0x4e, 0xf7);
// {0B390488-D80F-4a68-8408-48DC199F0E97}
DEFINE_GUID(CLSID_FFDSHOWRAW      , 0x0b390488, 0xd80f, 0x4a68, 0x84, 0x08, 0x48, 0xdc, 0x19, 0x9f, 0x0e, 0x97);
// {DBF9000E-F08C-4858-B769-C914A0FBB1D7}
DEFINE_GUID(CLSID_FFDSHOWSUBTITLES, 0xdbf9000e, 0xf08c, 0x4858, 0xb7, 0x69, 0xc9, 0x14, 0xa0, 0xfb, 0xb1, 0xd7);
// {0B0EFF97-C750-462c-9488-B10E7D87F1A6}
DEFINE_GUID(CLSID_FFDSHOWDXVA,      0xb0eff97, 0xc750, 0x462c, 0x94, 0x88, 0xb1, 0xe, 0x7d, 0x87, 0xf1, 0xa6);
// {996B7CE8-FE76-4da3-98C6-4E0046137019}
DEFINE_GUID(CLSID_FFDSHOWVFW      , 0x996b7ce8, 0xfe76, 0x4da3, 0x98, 0xc6, 0x4e, 0x0 , 0x46, 0x13, 0x70, 0x19);
// {9A98ADCC-C6A4-449e-A8B1-0363673D9F8A}
DEFINE_GUID(CLSID_TFFDSHOWPAGE    , 0x9a98adcc, 0xc6a4, 0x449e, 0xa8, 0xb1, 0x03, 0x63, 0x67, 0x3d, 0x9f, 0x8a);
// {0512B874-44F6-48f1-AFB5-6DE808DDE230}
DEFINE_GUID(CLSID_TFFDSHOWPAGERAW , 0x0512b874, 0x44f6, 0x48f1, 0xaf, 0xb5, 0x6d, 0xe8, 0x08, 0xdd, 0xe2, 0x30);
// {545A00C2-FCCC-40b3-9310-2C36AE64B0DD}
DEFINE_GUID(CLSID_TFFDSHOWPAGESUBTITLES, 0x545a00c2, 0xfccc, 0x40b3, 0x93, 0x10, 0x2c, 0x36, 0xae, 0x64, 0xb0, 0xdd);
// {D6A9B8CC-192D-4f00-8BF8-AD8774011B07}
DEFINE_GUID(CLSID_TFFDSHOWPAGEVFW , 0xd6a9b8cc, 0x192d, 0x4f00, 0x8b, 0xf8, 0xad, 0x87, 0x74, 0x01, 0x1b, 0x07);
// {650DE05E-5CD3-44f8-BA20-A5BB91FC61E6}
DEFINE_GUID(CLSID_TFFDSHOWPAGEPROC, 0x650de05e, 0x5cd3, 0x44f8, 0xba, 0x20, 0xa5, 0xbb, 0x91, 0xfc, 0x61, 0xe6);
// {A273C7F6-25D4-46b0-B2C8-4F7FADC44E37}
//DEFINE_TGUID(IID,IVMRffdshow9,      0xa273c7f6, 0x25d4, 0x46b0, 0xb2, 0xc8, 0x4f, 0x7f, 0xad, 0xc4, 0x4e, 0x37)
// {545A00C2-FCCC-40b3-9310-2C36AE64B0DD}
DEFINE_GUID(CLSID_TFFDSHOWPAGEDXVA, 0x545a00c2, 0xfccc, 0x40b3, 0x93, 0x10, 0x2c, 0x36, 0xae, 0x64, 0xb0, 0xdd);

#endif
