#ifndef _IFFDSHOWPARAMSENUM_H_
#define _IFFDSHOWPARAMSENUM_H_

// {BCF50246-9043-4c48-AB55-602F0CD35DB3}
DEFINE_GUID(IID_IffdshowParamsEnum, 0xbcf50246, 0x9043, 0x4c48, 0xab, 0x55, 0x60, 0x2f, 0x0c, 0xd3, 0x5d, 0xb3);

enum
{
 FFDSHOW_PARAM_INT,
 FFDSHOW_PARAM_STRING,
};
class TffdshowBase;
#pragma pack(push,1)
struct TffdshowParamInfo
{
 int id;
 const TCHAR *valnamePtr,*namePtr;
 char *valname,*name;
 int type;
 int min,max; //for integer
 int maxlen; //for string
 int inPreset;
 void *ptr;
 int isNotify;
};
#pragma pack(pop)

DECLARE_INTERFACE_(IffdshowParamsEnum,IUnknown)
{
 STDMETHOD (resetEnum)(void) PURE;
 STDMETHOD (nextEnum)(TffdshowParamInfo* paramPtr) PURE;
};

#endif
