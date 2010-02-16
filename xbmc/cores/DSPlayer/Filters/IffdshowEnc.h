#ifndef _IFFDSHOWENC_H_
#define _IFFDSHOWENC_H_

#define FFDSHOWENC_NAME_L L"FFDS"
#define FFDSHOWENC_DESC_L L"ffdshow video encoder"

struct TencStats;
class TffProcVideo;
struct Tencoder;
struct TcoSettings;
struct TvideoCodecs;
class Tco;
DECLARE_INTERFACE_(IffdshowEnc, IUnknown)
{
 STDMETHOD (getEncStats)(TencStats* *encStatsPtr) PURE;
 STDMETHOD (getFFproc)(TffProcVideo* *procPtr) PURE;
 STDMETHOD (isLAVCadaptiveQuant)(void) PURE;
 STDMETHOD (isQuantControlActive)(void) PURE;
 STDMETHOD (getCustomQuantMatrixes)(unsigned char* *intra8,unsigned char* *inter8,unsigned char* *intra4Y,unsigned char* *inter4Y,unsigned char* *intra4C,unsigned char* *inter4C) PURE;
 STDMETHOD (getEncoder)(int codecId,const Tencoder* *encPtr) PURE;
 STDMETHOD_(int,getQuantType2)(int quant) PURE;
 STDMETHOD (getCoSettingsPtr)(const TcoSettings* *coSettingsPtr) PURE;
 STDMETHOD (setCoSettingsPtr)(TcoSettings *coSettingsPtr) PURE;
 STDMETHOD (loadEncodingSettings)(void) PURE;
 STDMETHOD (saveEncodingSettings)(void) PURE;
 STDMETHOD_(int,loadEncodingSettingsMem)(const void *buf,int len) PURE;
 STDMETHOD_(int,saveEncodingSettingsMem)(void *buf,int len) PURE;
 STDMETHOD (showGraph)(HWND parent) PURE;
 STDMETHOD (getVideoCodecs)(const TvideoCodecs* *codecs) PURE;
 STDMETHOD (getCoPtr)(Tco* *coPtr) PURE;
 STDMETHOD (_release)(void) PURE;
 STDMETHOD (muxHeader)(const void *data,size_t datalen,int flush) PURE;
 STDMETHOD_(void,setHgraph)(HWND hwnd) PURE;
 STDMETHOD_(HWND,getHgraph)(void) PURE;
};

DECLARE_INTERFACE_(IffdshowEncVFW,IUnknown)
{
 STDMETHOD_(LRESULT,query)(const BITMAPINFOHEADER *inhdr,BITMAPINFOHEADER *outhdr) PURE;
 STDMETHOD_(LRESULT,getFormat)(const BITMAPINFOHEADER *inhdr,BITMAPINFO *lpbiOutput) PURE;
 STDMETHOD_(LRESULT,getSize)(const BITMAPINFO *lpbiInput) PURE;
 STDMETHOD_(LRESULT,begin)(const BITMAPINFOHEADER *inhdr) PURE;
 STDMETHOD_(LRESULT,end)(void) PURE;
 STDMETHOD_(LRESULT,compress)(const BITMAPINFOHEADER *inhdr,const unsigned char *src,size_t srclen,__int64 rtStart,__int64 rtStop) PURE;
 STDMETHOD_(void,setICC)(void *icc0) PURE;
};

#ifndef DEFINE_TGUID
 #define DEFINE_TGUID(IID,I, l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(IID##_##I,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif
// {CED9A2C3-8534-4bcc-AAB2-10E010001C28}
DEFINE_TGUID(IID,IffdshowEnc        , 0xced9a2c3, 0x8534, 0x4bcc, 0xaa, 0xb2, 0x10, 0xe0, 0x10, 0x00, 0x1c, 0x28)
// {9C4A4EC7-1E14-4cdc-90C3-DFC5E2930F93}
DEFINE_TGUID(IID,IffdshowEncVFW     , 0x9c4a4ec7, 0x1e14, 0x4cdc, 0x90, 0xc3, 0xdf, 0xc5, 0xe2, 0x93, 0x0f, 0x93)
// {4DB2B5D9-4556-4340-B189-AD20110D953F}
DEFINE_GUID(CLSID_FFDSHOWENC        , 0x4db2b5d9, 0x4556, 0x4340, 0xb1, 0x89, 0xad, 0x20, 0x11, 0x0d, 0x95, 0x3f);
// {1F71651E-65D2-40bf-AC44-275D11927D99}
DEFINE_GUID(CLSID_FFDSHOWENCVFW     , 0x1f71651e, 0x65d2, 0x40bf, 0xac, 0x44, 0x27, 0x5d, 0x11, 0x92, 0x7d, 0x99);
// {5711D95F-0984-4a22-8FF8-90A954958D0C}
DEFINE_GUID(CLSID_TFFDSHOWENCPAGE   , 0x5711d95f, 0x0984, 0x4a22, 0x8f, 0xf8, 0x90, 0xa9, 0x54, 0x95, 0x8d, 0x0c);
// {7CA71B1E-A67D-4d54-A200-FA47605483A7}
DEFINE_GUID(CLSID_TFFDSHOWENCPAGEVFW, 0x7ca71b1e, 0xa67d, 0x4d54, 0xa2, 0x00, 0xfa, 0x47, 0x60, 0x54, 0x83, 0xa7);

#endif
