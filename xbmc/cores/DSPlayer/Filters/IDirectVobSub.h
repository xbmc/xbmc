/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

interface __declspec(uuid("EBE1FB08-3957-47ca-AF13-5827E5442E56"))
IDirectVobSub :
public IUnknown {
    STDMETHOD(get_FileName)(THIS_ WCHAR * fn) PURE; // fn should point to a buffer allocated to at least the length of MAX_PATH (=260)

    STDMETHOD(put_FileName)(THIS_ WCHAR * fn) PURE;

    STDMETHOD(get_LanguageCount)(THIS_ int* nLangs) PURE;

    STDMETHOD(get_LanguageName)(THIS_ int iLanguage, WCHAR** ppName) PURE; // the returned *ppName is allocated with CoTaskMemAlloc

    STDMETHOD(get_SelectedLanguage)(THIS_ int* iSelected) PURE;

    STDMETHOD(put_SelectedLanguage)(THIS_ int iSelected) PURE;

    STDMETHOD(get_HideSubtitles)(THIS_ bool * fHideSubtitles) PURE;

    STDMETHOD(put_HideSubtitles)(THIS_ bool fHideSubtitles) PURE;

    // deprecated
    STDMETHOD(get_PreBuffering)(THIS_ bool * fDoPreBuffering) PURE;

    // deprecated
    STDMETHOD(put_PreBuffering)(THIS_ bool fDoPreBuffering) PURE;

    STDMETHOD(get_Placement)(THIS_ bool * fOverridePlacement, int* xperc, int* yperc) PURE;

    STDMETHOD(put_Placement)(THIS_ bool fOverridePlacement, int xperc, int yperc) PURE;

    STDMETHOD(get_VobSubSettings)(THIS_ bool * fBuffer, bool * fOnlyShowForcedSubs, bool * fPolygonize) PURE;

    STDMETHOD(put_VobSubSettings)(THIS_ bool fBuffer, bool fOnlyShowForcedSubs, bool fPolygonize) PURE;

    // depending on lflen, lf must point to LOGFONTA or LOGFONTW
    STDMETHOD(get_TextSettings)(THIS_ void* lf, int lflen, COLORREF * color, bool * fShadow, bool * fOutline, bool * fAdvancedRenderer) PURE;

    STDMETHOD(put_TextSettings)(THIS_ void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer) PURE;

    STDMETHOD(get_Flip)(THIS_ bool * fPicture, bool * fSubtitles) PURE;

    STDMETHOD(put_Flip)(THIS_ bool fPicture, bool fSubtitles) PURE;

    STDMETHOD(get_OSD)(THIS_ bool * fOSD) PURE;

    STDMETHOD(put_OSD)(THIS_ bool fOSD) PURE;

    STDMETHOD(get_SaveFullPath)(THIS_ bool * fSaveFullPath) PURE;

    STDMETHOD(put_SaveFullPath)(THIS_ bool fSaveFullPath) PURE;

    STDMETHOD(get_SubtitleTiming)(THIS_ int* delay, int* speedmul, int* speeddiv) PURE;

    STDMETHOD(put_SubtitleTiming)(THIS_ int delay, int speedmul, int speeddiv) PURE;

    STDMETHOD(get_MediaFPS)(THIS_ bool * fEnabled, double * fps) PURE;

    STDMETHOD(put_MediaFPS)(THIS_ bool fEnabled, double fps) PURE;

    // no longer supported

    STDMETHOD(get_ColorFormat)(THIS_ int* iPosition) PURE;

    STDMETHOD(put_ColorFormat)(THIS_ int iPosition) PURE;


    STDMETHOD(get_ZoomRect)(THIS_ NORMALIZEDRECT * rect) PURE;

    STDMETHOD(put_ZoomRect)(THIS_ NORMALIZEDRECT * rect) PURE;


    STDMETHOD(UpdateRegistry)(THIS_) PURE;


    STDMETHOD(HasConfigDialog)(THIS_ int iSelected) PURE;

    STDMETHOD(ShowConfigDialog)(THIS_ int iSelected, HWND hWndParent) PURE; // if available, this will popup a child dialog allowing the user to edit the style options


    STDMETHOD(IsSubtitleReloaderLocked)(THIS_ bool * fLocked) PURE;

    STDMETHOD(LockSubtitleReloader)(THIS_ bool fLock) PURE;

    STDMETHOD(get_SubtitleReloader)(THIS_ bool * fDisabled) PURE;

    STDMETHOD(put_SubtitleReloader)(THIS_ bool fDisable) PURE;

    // horizontal: 0 - disabled, 1 - mod32 extension (width = (width + 31) & ~31)
    // vertical:   0 - disabled, 1 - 16:9, 2 - 4:3, 0x80 - crop (use crop together with 16:9 or 4:3, eg 0x81 will crop to 16:9 if the picture was taller)
    // resx2:      0 - disabled, 1 - enabled, 2 - depends on the original resolution
    // resx2minw:  resolution doubler will be used if width * height <= resx2minw * resx2minh (resx2minw * resx2minh equals to 384 * 288 by default)
    STDMETHOD(get_ExtendPicture)(THIS_ int* horizontal, int* vertical, int* resx2, int* resx2minw, int* resx2minh) PURE;

    STDMETHOD(put_ExtendPicture)(THIS_ int horizontal, int vertical, int resx2, int resx2minw, int resx2minh) PURE;

    // level: 0 - when needed, 1 - always, 2 - disabled
    STDMETHOD(get_LoadSettings)(THIS_ int* level, bool * fExternalLoad, bool * fWebLoad, bool * fEmbeddedLoad) PURE;

    STDMETHOD(put_LoadSettings)(THIS_ int level, bool fExternalLoad, bool fWebLoad, bool fEmbeddedLoad) PURE;

    STDMETHOD(get_SubPictToBuffer)(THIS_ unsigned int* uSubPictToBuffer) PURE;

    STDMETHOD(put_SubPictToBuffer)(THIS_ unsigned int uSubPictToBuffer) PURE;

    STDMETHOD(get_AnimWhenBuffering)(THIS_ bool * fAnimWhenBuffering) PURE;

    STDMETHOD(put_AnimWhenBuffering)(THIS_ bool fAnimWhenBuffering) PURE;
};

[uuid("716d5167-2140-4e99-bbc9-4248a1008990")]
interface IDSPlayerCustom : public IUnknown
{
  // Set a custom callback function to handle the property page
  STDMETHOD(SetPropertyPageCallback)(HRESULT(*fpPropPageCallback)(IUnknown* pFilter)) = 0;
};

#ifdef __cplusplus
}
#endif
