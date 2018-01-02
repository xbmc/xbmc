/*****************************************************************
|
|   Neptune - WinRT Utilities
|
|   (c) 2001-2012 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"
#include "NptWinRtUtils.h"

/*----------------------------------------------------------------------
|   NPT_WinRtUtils::A2WHelper
+---------------------------------------------------------------------*/
LPWSTR
NPT_WinRtUtils::A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
    int ret;

    assert(lpa != NULL);
    assert(lpw != NULL);
    if (lpw == NULL || lpa == NULL) return NULL;

    lpw[0] = '\0';
    ret = MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
    if (ret == 0) {
        assert(0);
        return NULL;
    }        
    return lpw;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUtils::W2AHelper
+---------------------------------------------------------------------*/
LPSTR
NPT_WinRtUtils::W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
    int ret;

    assert(lpa != NULL);
    assert(lpw != NULL);
    if (lpa == NULL || lpw == NULL) return NULL;

    lpa[0] = '\0';
    ret = WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
    if (ret == 0) {
        int error = GetLastError();
        assert(error);
        return NULL;
    }
    return lpa;
}

/*----------------------------------------------------------------------
|   NPT_WinRtUtils::LStrLenW
+---------------------------------------------------------------------*/
size_t
NPT_WinRtUtils::LStrLenW(STRSAFE_LPCWSTR lpw)
{
	size_t len = 0;
	HRESULT result = StringCchLengthW(lpw, STRSAFE_MAX_CCH, &len);
	if (S_OK == result) {
		return len;
	} else {
		return INT_MAX;
	}
}

