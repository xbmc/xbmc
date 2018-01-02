/*****************************************************************
|
|   Neptune - WinRT Utilities
|
|   (c) 2001-2012 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
 |   NPT_WinRtUtils
 +---------------------------------------------------------------------*/
class NPT_WinRtUtils {
public:
    // unicode/ascii conversion
    static LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp);
    static LPSTR W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp);
    static size_t LStrLenW(STRSAFE_LPCWSTR lpw);
};

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
/* UNICODE support */
#define NPT_WINRT_USE_CHAR_CONVERSION int _convert = 0; LPCWSTR _lpw = NULL; LPCSTR _lpa = NULL

#define NPT_WINRT_A2W(lpa) (\
    ((_lpa = lpa) == NULL) ? NULL : (\
    _convert = (int)(strlen(_lpa)+1),\
    (INT_MAX/2<_convert)? NULL :  \
    NPT_WinRtUtils::A2WHelper((LPWSTR) alloca(_convert*sizeof(WCHAR)), _lpa, _convert, CP_UTF8)))

/* +2 instead of +1 temporary fix for Chinese characters */
#define NPT_WINRT_W2A(lpw) (\
    ((_lpw = lpw) == NULL) ? NULL : (\
    (_convert = (NPT_WinRtUtils::LStrLenW(_lpw)+2), \
    (_convert>INT_MAX/2) ? NULL : \
    NPT_WinRtUtils::W2AHelper((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR), CP_UTF8))))

