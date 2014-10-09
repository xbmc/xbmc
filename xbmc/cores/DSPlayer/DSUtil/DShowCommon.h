/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//////////////////////////////////////////////////////////////////////////
// DSUtil.h: DirectShow helper functions.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

// Conventions:
//
// Functions named "IsX" return true or false.
//
// Functions named "FindX" enumerate over a collection and return the first
// matching instance. 

#pragma once
/*
    Pin and Filter Functions
    -------------------
    GetPinCategory
    GetPinMediaType
  FilterSupportsPropertyPage
    FindUnconnectedPin
    FindConnectedPin
  FindPinByCategory
  FindPinByIndex
    FindPinByMajorType
    FindPinByName
    FindPinInterface
    FindMatchingPin
    IsPinConnected
    IsPinDirection
    IsPinUnconnected
    IsSourceFilter
    IsRenderer
    ShowFilterPropertyPage

    Graph Building Functions 
    -----------------------
    AddFilterByCLSID
  AddFilterFromMoniker
    AddSourceFilter
    AddVMR9Filter
    AddWriterFilter
    ConnectFilters
    CreateKernelFilter
    DisconnectPin
    FindFilterInterface
    FindGraphInterface
    GetNextFilter
    GetConnectedFilter
    RemoveFilter
    RemoveFiltersDownstream
    RemoveUnconnectedFilters
    RenderFileToVideoRenderer

    Media Type Functions
    --------------------
    CreatePCMAudioType
    CreateRGBVideoType
    DeleteMediaType
    FreeMediaType
    CopyFormatBlock

    GraphEdit Functions
    -------------------
    AddGraphToRot
    LoadGraphFile
    RemoveGraphFromRot
    SaveGraphFile

    Misc Functions
    --------------
    FramesPerSecToFrameLength
    LetterBoxRect
    MsecToRefTime
    RectWidth
    RectHeight
    RefTimeToMsec
    RefTimeToSeconds
    SecondsToRefTime

*/

#include "SmartPtr.h"
#include "strsafe.h"
#include <assert.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
{Com::SmartPtr<IEnumPins> pEnumPins; \
  if(pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
{ \
  for(Com::SmartPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = NULL) \
{ \

#define EndEnumPins }}}


const LONGLONG ONE_SECOND = 10000000;  // One second, in 100-nanosecond units.
const LONGLONG ONE_MSEC = ONE_SECOND / 10000;  // One millisecond, in 100-nanosecond units

// Directions for filter graph data flow.
enum GraphDirection
{ 
    UPSTREAM, DOWNSTREAM
};


// Forward declares 

void    _FreeMediaType(AM_MEDIA_TYPE& mt);
void    _DeleteMediaType(AM_MEDIA_TYPE *pmt);
HRESULT RemoveFiltersDownstream(IGraphBuilder *pGraph, IBaseFilter *pFilter);



/**********************************************************************

    Pin Query Functions - Test pins for various things

**********************************************************************/


///////////////////////////////////////////////////////////////////////
// Name: GetPinCategory
// Desc: Returns the category of a pin
//
// Note: Pin categories are used by some kernel-mode filters to
//       distinguish different outputs. (e.g, capture and preview)
///////////////////////////////////////////////////////////////////////

inline HRESULT GetPinCategory(IPin *pPin, GUID *pPinCategory)
{
    if (pPin == NULL)
    {
        return E_POINTER;
    }
    if (pPinCategory == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    DWORD cbReturned = 0;
    IKsPropertySet *pKs = NULL;

    CHECK_HR(hr = pPin->QueryInterface(IID_IKsPropertySet, (void**)&pKs));

    // Try to retrieve the pin category.
    CHECK_HR(hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, 
        pPinCategory, sizeof(GUID), &cbReturned));
    
    // If this succeeded, pPinCategory now contains the category GUID.


done:
    SAFE_RELEASE(pKs);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: GetPinMediaType
// Desc: Given a pin, find a preferred media type 
//
// pPin         Pointer to the pin
// majorType    Preferred major type (GUID_NULL = don't care)
// subType      Preferred subtype (GUID_NULL = don't care)
// formatType   Preferred format type (GUID_NULL = don't care)
// ppmt         Receives a pointer to the media type. Can be NULL.
//
// Note: If you want to check whether a pin supports a desired media type,
//       but do not need the format details, set ppmt to NULL.
//
//       If ppmt is not NULL and the method succeeds, the caller must
//       delete the media type, including the format block. (Use the
//       DeleteMediaType function.)
///////////////////////////////////////////////////////////////////////

inline HRESULT GetPinMediaType(
    IPin *pPin,             // pointer to the pin
    REFGUID majorType,      // desired major type, or GUID_NULL = don't care
    REFGUID subType,        // desired subtype, or GUID_NULL = don't care
    REFGUID formatType,     // desired format type, of GUID_NULL = don't care
    AM_MEDIA_TYPE **ppmt    // Receives a pointer to the media type. (Can be NULL)
    )
{
    if (!pPin)
    {
        return E_POINTER;
    }

    IEnumMediaTypes *pEnum = NULL;
    AM_MEDIA_TYPE *pmt = NULL;

    HRESULT hr = S_OK;
    bool    bFound = false;
    
    CHECK_HR(hr = pPin->EnumMediaTypes(&pEnum));

    while (hr = pEnum->Next(1, &pmt, NULL), hr == S_OK)
    {
        if ((majorType == GUID_NULL) || (majorType == pmt->majortype))
        {
            if ((subType == GUID_NULL) || (subType == pmt->subtype))
            {
                if ((formatType == GUID_NULL) || (formatType == pmt->formattype))
                {
                    // Found a match. 
                    if (ppmt)
                    {
                        *ppmt = pmt;  // Return it to the caller
                    }
                    else
                    {
                        _DeleteMediaType(pmt);
                    }
                    bFound = true;
                    break;
                }
            }
        }
        _DeleteMediaType(pmt);
    }

done:
    SAFE_RELEASE(pEnum);
    if (SUCCEEDED(hr))
    {
        if (!bFound)
        {
            hr = VFW_E_NOT_FOUND;
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: IsPinConnected
// Desc: Query whether a pin is connected to another pin.
//
// Note: If you need to get the other pin, use IPin::ConnectedTo.
///////////////////////////////////////////////////////////////////////

inline HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
    if (pPin == NULL || pResult == NULL)
    {
        return E_POINTER;
    }

    IPin *pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    SAFE_RELEASE(pTmp);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: IsPinUnconnected
// Desc: Query whether a pin is NOT connected to another pin.
//
///////////////////////////////////////////////////////////////////////

inline HRESULT IsPinUnconnected(IPin *pPin, BOOL *pResult)
{
    // Check if the pin connected.
    HRESULT hr = IsPinConnected(pPin, pResult);
    if (SUCCEEDED(hr))
    {
        // Reverse the result.
        *pResult = !(*pResult);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: IsPinDirection
// Desc: Query whether a pin has a specified direction (input / output)
//
///////////////////////////////////////////////////////////////////////

inline HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
    if (pPin == NULL || pResult == NULL)
    {
        return E_POINTER;
    }

    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}


/**********************************************************************

    Function objects

    These are used in some of the FindXXXX functions.

**********************************************************************/

// MatchPinName: 
// Function object to match a pin by name.
struct MatchPinName
{
    const WCHAR *m_wszName;

    MatchPinName(const WCHAR *wszName)
    {
        m_wszName = wszName;
    }

    HRESULT operator()(IPin *pPin, BOOL *pResult)
    {
        assert(pResult != NULL);

        PIN_INFO PinInfo;
        HRESULT hr = pPin->QueryPinInfo(&PinInfo);
        if (SUCCEEDED(hr))
        {
            PinInfo.pFilter->Release();

            // TODO: Use Strsafe
            if (wcscmp(m_wszName, PinInfo.achName) == 0)
            {
                *pResult = TRUE;
            }
            else
            {
                *pResult = FALSE;
            }
        }
        return hr;
    }
};


// MatchPinDirectionAndConnection
// Function object to match a pin by direction and connection

struct MatchPinDirectionAndConnection
{
    BOOL            m_bShouldBeConnected;
    PIN_DIRECTION   m_direction;


    MatchPinDirectionAndConnection(PIN_DIRECTION direction, BOOL bShouldBeConnected) 
        : m_bShouldBeConnected(bShouldBeConnected), m_direction(direction)
    {
    }

    HRESULT operator()(IPin *pPin, BOOL *pResult)
    {
        assert(pResult != NULL);

        BOOL bMatch = FALSE;
        BOOL bIsConnected = FALSE;

        HRESULT hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr))
        {
            if (bIsConnected == m_bShouldBeConnected)
            {
                hr = IsPinDirection(pPin, m_direction, &bMatch);
            }
        }

        if (SUCCEEDED(hr))
        {
            *pResult = bMatch;
        }
        return hr;
    }
};


// MatchPinConnection
// Function object to match a pin connection status

struct MatchPinConnection
{
    BOOL            m_bShouldBeConnected;

    MatchPinConnection(BOOL bShouldBeConnected) 
        : m_bShouldBeConnected(bShouldBeConnected)
    {
    }

    HRESULT operator()(IPin *pPin, BOOL *pResult)
    {
        assert(pResult != NULL);

        BOOL bIsConnected = FALSE;

        HRESULT hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr))
        {
            *pResult = (bIsConnected == m_bShouldBeConnected);
        }
        return hr;
    }
};

// MatchPinDirectionAndCategory
// Function object to match a pin by direction and category.
struct MatchPinDirectionAndCategory
{
  const GUID*    m_pCategory;
    PIN_DIRECTION   m_direction;

  MatchPinDirectionAndCategory(PIN_DIRECTION direction, REFGUID guidCategory)
    : m_direction(direction), m_pCategory(&guidCategory)
  {
  }

    HRESULT operator()(IPin *pPin, BOOL *pResult)
    {
        assert(pResult != NULL);

        BOOL bMatch = FALSE;
    GUID category;

        HRESULT hr = IsPinDirection(pPin, m_direction, &bMatch);
      
        if (SUCCEEDED(hr) && bMatch)
        {
      hr = GetPinCategory(pPin, &category);
      if (SUCCEEDED(hr))
      {
        bMatch = (category == *m_pCategory);
      }
        }

        if (SUCCEEDED(hr))
        {
            *pResult = bMatch;
        }
        return hr;
    }
};


struct MatchPinMediaType
{
    GUID            m_majorType;
    MatchPinDirectionAndConnection  m_match1;

    MatchPinMediaType(REFGUID majorType, PIN_DIRECTION dir, BOOL bShouldBeConnected)
        : m_majorType(majorType), m_match1(dir, bShouldBeConnected)
    {
    }

    HRESULT operator()(IPin *pPin, BOOL *pResult)
    {
        assert(pResult != NULL);

        HRESULT hr = S_OK;
        BOOL bMatch = FALSE;

        // First try to match on direction and connection status.
        hr = m_match1(pPin, &bMatch);

        // Next, try to match media types.
        if (SUCCEEDED(hr) && bMatch)
        {
            // For a connected pin, try to match on the
            // media type for the pin connection. Otherwise,
            // try to match on the preferred media type.

            const BOOL bConnected = m_match1.m_bShouldBeConnected;

            if (bConnected)
            {
                AM_MEDIA_TYPE mt = { 0 };

                hr = pPin->ConnectionMediaType(&mt);

                if (SUCCEEDED(hr))
                {
                    bMatch = (mt.majortype == m_majorType);
                    _FreeMediaType(mt);
                }
            }
            else
            {
                hr = GetPinMediaType(pPin, m_majorType, GUID_NULL, GUID_NULL, NULL);

                if (hr == VFW_E_NOT_FOUND)
                {
                    bMatch = FALSE;
                    hr = S_OK;  // Not a failure case, just no match.
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            *pResult = bMatch;
        }
        return hr;
    }
};

/**************************************************************************

    Pin Searching Functions

    These functions search a filter for a pin that matches some set of
    criteria. They return the first pin that matches, or VFW_E_NOT_FOUND.

**************************************************************************/



///////////////////////////////////////////////////////////////////////
// Name: FindPinInterface
// Desc: Search a filter for a pin that exposes a specified interface.
//       (Returns the first instance found.)
// 
// pFilter  Pointer to the filter to search.
// iid      IID of the interface.
// ppUnk    Receives the interface pointer.
// Q        Address of an ATL smart pointer.
//
// Note:    This function returns the first instance that it finds. 
//          If no pin is found, the function returns VFW_E_NOT_FOUND.
//          The templated version deduces the IID.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindPinInterface(
    IBaseFilter *pFilter,  // Pointer to the filter to search.
    REFGUID iid,           // IID of the interface.
    void **ppUnk)          // Receives the interface pointer.
{
    if (!pFilter || !ppUnk) 
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    bool bFound = false;

    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    CHECK_HR(hr = pFilter->EnumPins(&pEnum));

    // Query every pin for the interface.
    while (S_OK == pEnum->Next(1, &pPin, 0))
    {
        hr = pPin->QueryInterface(iid, ppUnk);
        SAFE_RELEASE(pPin);
        if (SUCCEEDED(hr))
        {
            bFound = true;
            break;
        }
    }

done:
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pPin);

    return (bFound ? S_OK : VFW_E_NOT_FOUND);
}
template <class Q>
HRESULT FindPinInterface(IBaseFilter *pFilter, Q** pp)
{
    return FindPinInterface(pFilter, __uuidof(Q), (void**)pp);
}



///////////////////////////////////////////////////////////////////////
// Name: FindMatchingPin (template)
// Desc: Return the first pin on a filter that matches a caller-supplied 
//       function or function object
//
// FN must be either
//   (a) A function with the signature HRESULT (IPin*, BOOL *)
//   (b) A class that implements HRESULT operator()(IPin*, BOOL *)
//
// FindMatchingPin halts if FN fails or returns TRUE
///////////////////////////////////////////////////////////////////////

template <class PinPred>
HRESULT FindMatchingPin(IBaseFilter *pFilter, PinPred FN, IPin **ppPin)
{
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    BOOL bFound = FALSE;
    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {
        hr = FN(pPin, &bFound);

        if (FAILED(hr))
        {
            pPin->Release();
            break;
        }
        if (bFound)
        {
            *ppPin = pPin;
            break;
        }

        pPin->Release();
    }

    pEnum->Release();

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////
// Name: FindPinByIndex
// Desc: Return the Nth pin with the specified pin direction.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindPinByIndex(IBaseFilter *pFilter, PIN_DIRECTION PinDir,
  UINT nIndex, IPin **ppPin)
{
  if (!pFilter || !ppPin)
  {
    return E_POINTER;
  }

    HRESULT hr = S_OK;
  bool bFound = false;
  UINT count = 0;

    IEnumPins *pEnum = NULL;
  IPin *pPin = NULL;

  CHECK_HR(hr = pFilter->EnumPins(&pEnum));

  while (S_OK == (hr = pEnum->Next(1, &pPin, NULL)))
  {
    PIN_DIRECTION ThisDir;
    CHECK_HR(hr = pPin->QueryDirection(&ThisDir));

    if (ThisDir == PinDir)
    {
      if (nIndex == count)
      {
        *ppPin = pPin;      // return to caller
                (*ppPin)->AddRef();
        bFound = true;
        break;
      }
      count++;
    }
    SAFE_RELEASE(pPin);
  }

done:
    SAFE_RELEASE(pPin);
  SAFE_RELEASE(pEnum);

    return (bFound ? S_OK : VFW_E_NOT_FOUND);
}

///////////////////////////////////////////////////////////////////////
// Name: FindUnconnectedPin
// Desc: Return the first unconnected input pin or output pin.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin            // Receives a pointer to the pin.
    )
{
    return FindMatchingPin(pFilter, MatchPinDirectionAndConnection(PinDir, FALSE), ppPin);
}



///////////////////////////////////////////////////////////////////////
// Name: FindConnectedPin
// Desc: Return the first connected input pin or output pin
///////////////////////////////////////////////////////////////////////

inline HRESULT FindConnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin            // Receives a pointer to the pin.
    )
{
    return FindMatchingPin(pFilter, MatchPinDirectionAndConnection(PinDir, TRUE), ppPin);
}


///////////////////////////////////////////////////////////////////////
// Name: FindPinByCategory
// Desc: Find the first pin that matches a specified pin category
//       and direction.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindPinByCategory(
  IBaseFilter *pFilter,   // Pointer to the filter.
  REFGUID guidCategory,   // Category GUID
  PIN_DIRECTION PinDir,   // Pin direction to match.
  IPin **ppPin
  )
{
  return FindMatchingPin(pFilter, MatchPinDirectionAndCategory(PinDir, guidCategory), ppPin);
}




///////////////////////////////////////////////////////////////////////
// Name: FindPinByName
// Desc: Find a pin by name.
//
// Note: Generally, you should find pins by direction, media type,
//       and/or pin category. However, in some cases you may need to
//       find a pin by name.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindPinByName(IBaseFilter *pFilter, const WCHAR *wszName, IPin **ppPin)
{
    if (!pFilter || !wszName || !ppPin)
    {
        return E_POINTER;
    }

    // Verify that wszName is not longer than MAX_PIN_NAME
    size_t cch;
    HRESULT hr = StringCchLengthW(wszName, MAX_PIN_NAME, &cch);

    if (SUCCEEDED(hr))
    {
        hr = FindMatchingPin(pFilter, MatchPinName(wszName), ppPin);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////
// Name: FindPinByMajorType
// Desc: Find a pin that matches a major media type GUID.
//
// This function also matches my pin direction and pin connection
// status. Hypothetically, this function could allow "don't care"
// values for these. But in my experience, you generally know this
// information for real-world uses (eg., if you are building the graph, 
// versus finding pins on a completed graph).
///////////////////////////////////////////////////////////////////////

inline HRESULT FindPinByMajorType(
    IBaseFilter     *pFilter,   // Pointer to the filter.
    REFGUID         majorType,  // Major media type.
    PIN_DIRECTION   PinDir,     // Pin direction to match.
    BOOL            bConnected, // If TRUE, look for connected pins.
                                // Otherwise, look for unconnected pins.
    IPin            **ppPin     // Receives a pointer to the pin.
    )
{
    if (!pFilter || !ppPin)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    hr = FindMatchingPin(pFilter, MatchPinMediaType(majorType, PinDir, bConnected), ppPin);

    return hr;
}

/**********************************************************************

    Filter Query Functions  - Test filters for various conditions.

**********************************************************************/

///////////////////////////////////////////////////////////////////////
// Name: IsSourceFilter
// Desc: Query whether a filter is a source filter.
///////////////////////////////////////////////////////////////////////

inline HRESULT IsSourceFilter(IBaseFilter *pFilter, BOOL *pResult)
{
    if (pFilter == NULL || pResult == NULL)
    {
        return E_POINTER;
    }

    IAMFilterMiscFlags *pFlags = NULL;
    IFileSourceFilter *pFileSrc = NULL;

    BOOL bIsSource = FALSE;

    // If the filter exposes the IAMFilterMiscFlags interface, we use
    // IAMFilterMiscFlags::GetMiscFlags to test if this is a source filter.

    // Otherwise, it is a source filter if it exposes IFileSourceFilter.

    // First try IAMFilterMiscFlags
    HRESULT hr = pFilter->QueryInterface(IID_IAMFilterMiscFlags, (void**)&pFlags);
    if (SUCCEEDED(hr))
    {

        ULONG flags = pFlags->GetMiscFlags();
        if (flags &  AM_FILTER_MISC_FLAGS_IS_SOURCE)
        {
            bIsSource = TRUE;
        }
    }
    else
    {
        // Next, look for IFileSourceFilter. 
        hr = pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pFileSrc);
        if (SUCCEEDED(hr))
        {
            bIsSource = TRUE;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bIsSource;
    }

    SAFE_RELEASE(pFlags);
    SAFE_RELEASE(pFileSrc);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: IsRenderer
// Desc: Query whether a filter is a renderer filter.
///////////////////////////////////////////////////////////////////////

inline HRESULT IsRenderer(IBaseFilter *pFilter, BOOL *pResult)
{
    if (pFilter == NULL || pResult == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    BOOL bIsRenderer = FALSE;

    IAMFilterMiscFlags *pFlags = NULL;
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;


    // First try IAMFilterMiscFlags. 
    hr = pFilter->QueryInterface(IID_IAMFilterMiscFlags, (void**)&pFlags);
    if (SUCCEEDED(hr))
    {
        ULONG flags = pFlags->GetMiscFlags();
        if (flags & AM_FILTER_MISC_FLAGS_IS_RENDERER)
        {
            bIsRenderer = TRUE;
        }
    }

    if (!bIsRenderer)
    {
        // Look for the following conditions:

        // 1) Zero output pins AND at least 1 unmapped input pin
        // - or -
        // 2) At least 1 rendered input pin.

        // definitions:
        // unmapped input pin = IPin::QueryInternalConnections returns E_NOTIMPL
        // rendered input pin = IPin::QueryInternalConnections returns "0" slots

        // These cases are somewhat obscure and probably don't apply to many filters
        // that actually exist.

        hr = pFilter->EnumPins(&pEnum);
        if (SUCCEEDED(hr))
        {
            bool bFoundRenderedInputPin = false;
            bool bFoundUnmappedInputPin = false;
            bool bFoundOuputPin = false;

            while (pEnum->Next(1, &pPin, NULL) == S_OK)
            {
                BOOL bIsOutput = FALSE;
                hr = IsPinDirection(pPin, PINDIR_OUTPUT, &bIsOutput);
                if (FAILED(hr))
                {
                    break;
                }
                else if (bIsOutput)
                {
                    // This is an output pin.
                    bFoundOuputPin = true;
                }
                else
                {
                    // It's an input pin. Is it mapped to an output pin?
                    ULONG nPin = 0;
                    hr = pPin->QueryInternalConnections(NULL, &nPin);
                    if (hr == S_OK)
                    {
                        // The count (nPin) was zero, and the method returned S_OK, so
                        // this input pin is mapped to exactly zero ouput pins. 
                        // Therefore, it is a rendered input pin. 
                        bFoundRenderedInputPin = true;

                        // We have met condition (2) above, so we can stop looking.
                        break;

                        // Note: S_FALSE here means "the count (nPin) was not large
                        // enough", ie, this pin is mapped to one or more output pins.

                    }
                    else if (hr == E_NOTIMPL)
                    {
                        // This pin is not mapped to any particular output pin. 
                        bFoundUnmappedInputPin = true;
                        hr = S_OK;
                    }
                    else if (FAILED(hr))
                    {
                        // Unexpected error
                        break;
                    }

                }
                pPin->Release();  // Release for the next loop.
            }  // while

           
            if (bFoundRenderedInputPin)
            {
                bIsRenderer = TRUE; // condition (1) above
            }
            else if (bFoundUnmappedInputPin && !bFoundOuputPin)
            {
                bIsRenderer = TRUE;  // condition (2) above
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bIsRenderer;
    }

    SAFE_RELEASE(pFlags);
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pPin);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: SupportsPropertyPage
// Desc: Query whether a filter has a property page.
///////////////////////////////////////////////////////////////////////

inline BOOL FilterSupportsPropertyPage(IBaseFilter *pFilter)
{
  if (pFilter == NULL)
  {
    return FALSE; 
  }

  ISpecifyPropertyPages *pProp = NULL;
  HRESULT hr = S_OK;
    
    hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, 
    (void**)&pProp);

    SAFE_RELEASE(pProp);
  return (SUCCEEDED(hr));
}


///////////////////////////////////////////////////////////////////////
// Name: ShowFilterPropertyPage
// Desc: Show a filter's property page.
//
// hwndParent: Owner window for the property page.
///////////////////////////////////////////////////////////////////////

inline HRESULT ShowFilterPropertyPage(IBaseFilter *pFilter, HWND hwndParent)
{
    HRESULT hr;

    ISpecifyPropertyPages *pSpecify = NULL;
  FILTER_INFO FilterInfo;
  CAUUID caGUID;

    if (!pFilter)
  {
        return E_POINTER;
  }

  ZeroMemory(&FilterInfo, sizeof(FilterInfo));
  ZeroMemory(&caGUID, sizeof(caGUID));

    // Discover if this filter contains a property page
    CHECK_HR(hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify));

    CHECK_HR(hr = pFilter->QueryFilterInfo(&FilterInfo));

  CHECK_HR(hr = pSpecify->GetPages(&caGUID));

  // Display the filter's property page
  CHECK_HR(hr = OleCreatePropertyFrame(
      hwndParent,             // Parent window
      0,                      // x (Reserved)
      0,                      // y (Reserved)
      FilterInfo.achName,     // Caption for the dialog box
      1,                      // Number of filters
      (IUnknown **)&pFilter,  // Pointer to the filter 
      caGUID.cElems,          // Number of property pages
      caGUID.pElems,          // Pointer to property page CLSIDs
      0,                      // Locale identifier
      0,                      // Reserved
      NULL                    // Reserved
      ));

done:
  CoTaskMemFree(caGUID.pElems);
  SAFE_RELEASE(FilterInfo.pGraph); 
    SAFE_RELEASE(pSpecify);
    return hr;
}

/**************************************************************************

    Graph Building Helper Functions

**************************************************************************/

///////////////////////////////////////////////////////////////////////
// Name: DisconnectPin
// Desc: Disconnect a pin from its peer.
//
// Note: If the pin is not connected, the function returns S_OK (no-op).
///////////////////////////////////////////////////////////////////////

inline HRESULT DisconnectPin(IGraphBuilder *pGraph, IPin *pPin)
{
  if (!pGraph || !pPin)
  {
    return E_POINTER;
  }

  HRESULT hr = S_OK;
  Com::SmartPtr<IPin> pPinTo = NULL;

  hr = pPin->ConnectedTo(&pPinTo);
  if (hr == VFW_E_NOT_CONNECTED)
  {
    // This pin is not connected.
    return S_OK; // no-op
  }

  // Disconnect the first pin.

  if (SUCCEEDED(hr))
  {
    hr = pGraph->Disconnect(pPin);
    if (SUCCEEDED(hr))
    {
      // Disconnect the other pin.
      hr = pGraph->Disconnect(pPinTo);
    }
  }

  return hr;
}



///////////////////////////////////////////////////////////////////////
// Name: RemoveFilter
// Desc: Removes a filter from the graph.
//
// Note: The function first disconnects the filter's pins. If the method
//       fails, some pins may be disconnected, so the graph is in an
//       unknown state.
///////////////////////////////////////////////////////////////////////

inline HRESULT RemoveFilter(IGraphBuilder *pGraph, IBaseFilter *pFilter)
{
    if (!pGraph || !pFilter)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    BeginEnumPins(pFilter, pEP, pPin)
    {
      CHECK_HR(hr = DisconnectPin(pGraph, pPin));
    }
    EndEnumPins;

    // Disconnect all the pins
    CHECK_HR(hr = pGraph->RemoveFilter(pFilter));

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////
// Name: AddFilterByCLSID
// Desc: Create a filter by CLSID and add it to the graph.
///////////////////////////////////////////////////////////////////////

inline HRESULT AddFilterByCLSID(
    IGraphBuilder *pGraph,          // Pointer to the Filter Graph Manager.
    const GUID& clsid,              // CLSID of the filter to create.
    IBaseFilter **ppF,              // Receives a pointer to the filter.
    LPCWSTR wszName = NULL          // A name for the filter (can be NULL).
    )
{
    if (!pGraph || ! ppF) 
    {
        return E_POINTER;
    }

    *ppF = 0;

    IBaseFilter *pFilter = NULL;
    HRESULT hr = S_OK;
    
    CHECK_HR(hr = CoCreateInstance(
        clsid, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IBaseFilter, 
        (void**)&pFilter));

    CHECK_HR(hr = pGraph->AddFilter(pFilter, wszName));

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
    SAFE_RELEASE(pFilter);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: AddFilterFromMoniker
// Desc: Create a filter from an IMoniker pointer and add it to the graph.
///////////////////////////////////////////////////////////////////////

inline HRESULT AddFilterFromMoniker(
    IGraphBuilder *pGraph,          // Pointer to the Filter Graph Manager.
  IMoniker *pFilterMoniker,    // Pointer to the moniker.
    IBaseFilter **ppF,              // Receives a pointer to the filter.
    LPCWSTR wszName                // A name for the filter (can be NULL).
  )
{
  if (!pGraph || !pFilterMoniker || !ppF)
  {
    return E_POINTER;
  }

    HRESULT hr = S_OK;
  IBaseFilter *pFilter = NULL;

  // Use the moniker to create the filter
    CHECK_HR(hr = pFilterMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pFilter));   
    
  // Add the capture filter to the filter graph
  CHECK_HR(hr = pGraph->AddFilter(pFilter, wszName));

  // Return to the caller.
  *ppF = pFilter;
  (*ppF)->AddRef();

done:
  SAFE_RELEASE(pFilter);
  return hr;
}




///////////////////////////////////////////////////////////////////////
// Name: AddSourceFilter
// Desc: Loads a source filter.
//
// pGraph: Pointer to the filter graph manager.
// szFile: File name.
// clsid:  CLSID of the source filter to use.
// ppSourceFilter: Receives a pointer to the source filter's IBaseFilter 
//                 interface.
//
// This function creates the filter, adds it to the graph, queries for
// IFileSourceFilter, and calls IFileSourceFilter::Load. 
//
// You can use this function if you want to specify the source filter by
// CLSID. Otherwise, just use IGraphBuilder::AddSourceFilter.
///////////////////////////////////////////////////////////////////////

inline HRESULT AddSourceFilter(
    IGraphBuilder *pGraph,          // Pointer to the filter graph manager.
    const WCHAR *szFile,            // File name
    const GUID& clsidSourceFilter,  // CLSID of the source filter to use
    IBaseFilter **ppSourceFilter)   // receives a pointer to the filter.
{
    if (!pGraph || !szFile || !ppSourceFilter)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IBaseFilter *pSource = NULL;
    IFileSourceFilter *pFileSource = NULL;

    // Create the source filter and add it to the graph.
    CHECK_HR(hr = CoCreateInstance(
        clsidSourceFilter, 
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**)&pSource));


    CHECK_HR(hr = pGraph->AddFilter(pSource, szFile));
    CHECK_HR(hr = pSource->QueryInterface(IID_IFileSourceFilter, (void**)&pFileSource));
    CHECK_HR(hr = pFileSource->Load(szFile, NULL));

    // Return the filter pointer to the caller.
    *ppSourceFilter = pSource;
    (*ppSourceFilter)->AddRef();

done:
    if (FAILED(hr))
    {
        // FAILED, remove the filter
        if (pSource != NULL)
        {
            RemoveFilter(pGraph, pSource);
        }
    }

    SAFE_RELEASE(pSource);
    SAFE_RELEASE(pFileSource);
    return hr;
}

///////////////////////////////////////////////////////////////////////
// Name: AddWriterFilter
// Desc: Adds a file-writing filter. (ie, a filter that supports
//       IFileSinkFilter.)
//
// pGraph: Pointer to the filter graph manager.
// szFile: File name.
// clsid:  CLSID of the filter.
// bOverwrite: If TRUE, overwrite the file.
// ppSourceFilter: Receives the filter's IBaseFilter interface.
//
// This function creates the filter, adds it to the graph, sets the 
// file name, and set the overwrite mode. 
///////////////////////////////////////////////////////////////////////

inline HRESULT AddWriterFilter(
    IGraphBuilder *pGraph,
    const WCHAR *szFile,
    const GUID& clsid,
  BOOL bOverwrite,
    IBaseFilter **ppFilter
    )
{
    if (pGraph == NULL)
    {
        return E_POINTER;
    }
    if (szFile == NULL)
    {
        return E_POINTER;
    }
    if (ppFilter == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IBaseFilter *pFilter = NULL;
    IFileSinkFilter *pSink = NULL;
  IFileSinkFilter2 *pSink2 = NULL;

    CHECK_HR(hr = AddFilterByCLSID(pGraph, clsid, &pFilter, szFile));

    CHECK_HR(hr = pFilter->QueryInterface(IID_IFileSinkFilter, (void**)&pSink));
    CHECK_HR(hr = pSink->SetFileName((LPCOLESTR)szFile, NULL));

  if (bOverwrite)
  {
        // Try to use IFileSinkFilter2 to set the file-overwrite mode, but if the filter 
        // does not implement IFileSinkFilter2, just ignore it.
    if (SUCCEEDED(pFilter->QueryInterface(IID_IFileSinkFilter2, (void**)&pSink2)))
        {
      CHECK_HR(hr = pSink2->SetMode(AM_FILE_OVERWRITE));
    }
  }

    *ppFilter = pFilter;
    (*ppFilter)->AddRef();

done:
    if (FAILED(hr))
    {
        RemoveFilter(pGraph, pFilter);
    }

  SAFE_RELEASE(pFilter);
  SAFE_RELEASE(pSink);
  SAFE_RELEASE(pSink2);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: ConnectFilters
// Desc: Connect two filters.
//
// There are 3 overrides on this method:
// (1) From output pin to filter
// (2) From filter to filter
// (3) From filter to input pin
// 
// Note: The 4th combination (output pin to input pin) is already
// provided by the IGraphBuilder::Connect method. 
//
// All of these methods search the filter(s) for the first unconnected
// pin. If that pin fails, the method fails. In some cases, it may be 
// better to try every pin until one succeeds, or loop through the pins 
// and examine the preferred media types. 
// 
///////////////////////////////////////////////////////////////////////

// ConnectFilters: Filter to pin

inline HRESULT ConnectFilters(
    IGraphBuilder *pGraph, // Filter Graph Manager.
    IPin *pOut,            // Output pin on the upstream filter.
    IBaseFilter *pDest)    // Downstream filter.
{
    if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }

    IPin *pIn = NULL;
    HRESULT hr = S_OK; 
        
    // Find an input pin on the downstream filter.
    CHECK_HR(hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn));

  // Try to connect them.
    CHECK_HR(hr = pGraph->Connect(pOut, pIn));

done:
  SAFE_RELEASE(pIn);
  return hr;
}

// ConnectFilters: Filter to filter

inline HRESULT ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IBaseFilter *pDest)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    CHECK_HR(hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut));
    CHECK_HR(hr = ConnectFilters(pGraph, pOut, pDest));

done:
    SAFE_RELEASE(pOut);
    return hr;
}

// ConnectFilters: Filter to pin

inline HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pIn == NULL))
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IPin *pOut = NULL;
        
    // Find an output pin on the upstream filter.
    CHECK_HR(hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut));

    // Try to connect them.
    CHECK_HR(hr = pGraph->Connect(pOut, pIn));

done:
  SAFE_RELEASE(pOut);
    return hr;
}
 


///////////////////////////////////////////////////////////////////////
// Name: CreateKernelFilter
// Desc: Create a kernel-mode filter by name.
///////////////////////////////////////////////////////////////////////

inline HRESULT CreateKernelFilter(
    const GUID &guidCategory,  // Filter category.
    LPCOLESTR szName,          // The name of the filter.
    IBaseFilter **ppFilter     // Receives a pointer to the filter.
)
{
    if (!szName || !ppFilter) 
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    bool bFound = false;

    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    IMoniker *pMoniker = NULL;

    // Create the system device enumerator.
    CHECK_HR(hr = CoCreateInstance(CLSID_SystemDeviceEnum, 
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum,
        (void**)&pDevEnum));

    // Create a class enumerator for the specified category.
    CHECK_HR(hr = pDevEnum->CreateClassEnumerator(guidCategory, &pEnum, 0));
    if (hr != S_OK) // S_FALSE means the category is empty.
    {
        CHECK_HR(hr = E_FAIL);
    }

    // Enumerate devices within this category.
    while (!bFound && (S_OK == pEnum->Next(1, &pMoniker, 0)))
    {
        // Get the property bag
        IPropertyBag *pBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pMoniker);
            continue; // Maybe the next one will work.
        }

        // Check the friendly name.
        VARIANT var;
        VariantInit(&var);
        hr = pBag->Read(L"FriendlyName", &var, NULL);
        if (SUCCEEDED(hr) && (lstrcmpiW(var.bstrVal, szName) == 0))
        {
            // This is the right filter.
            hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter,
                (void**)ppFilter);
            bFound = true;
        }
        VariantClear(&var);
        SAFE_RELEASE(pBag);
        SAFE_RELEASE(pMoniker);
    
    } // while
    
    if (!bFound) 
    {
        hr = E_FAIL;
    }

done:
    SAFE_RELEASE(pDevEnum);
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pMoniker);
    return hr;
}




///////////////////////////////////////////////////////////////////////
// Name: FindFilterInterface
// Desc: Search the graph for a filter that exposes a specified interface.
// 
// pGraph   Pointer to the Filter Graph Manager.
// iid      IID of the interface to retrieve.
// ppUnk    Receives the interface pointer.
// Q        Address of an ATL smart pointer.
//
// Note:    This function returns the first instance that it finds. 
//          If no filter is found, the function returns VFW_E_NOT_FOUND.
//          The templated version takes an ATL smart pointer and deduces the IID.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindFilterInterface(
    IGraphBuilder *pGraph, // Pointer to the Filter Graph Manager.
    REFGUID iid,           // IID of the interface to retrieve.
    void **ppUnk)          // Receives the interface pointer.
{
    if (!pGraph || !ppUnk) 
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    bool bFound = false;

    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;

    CHECK_HR(hr = pGraph->EnumFilters(&pEnum));

    // Query every filter for the interface.
    while (S_OK == pEnum->Next(1, &pFilter, 0))
    {
        hr = pFilter->QueryInterface(iid, ppUnk);
        SAFE_RELEASE(pFilter);
        if (SUCCEEDED(hr))
        {
            bFound = true;
            break;
        }
    }

done:
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pFilter);

    return (bFound ? S_OK : VFW_E_NOT_FOUND);
}

template <class Q>
HRESULT FindFilterInterface(IGraphBuilder *pGraph, Q** pp)
{
    return FindFilterInterface(pGraph, __uuidof(Q), (void**)pp);
}




///////////////////////////////////////////////////////////////////////
// Name: FindGraphInterface
// Desc: Search the graph for a filter OR pin that exposes a specified 
//       interface. 
// 
// pGraph   Pointer to the Filter Graph Manager.
// iid      IID of the interface.
// ppUnk    Receives the interface pointer.
// Q        Address of an ATL smart pointer.
//
// Note:    This function returns the first instance that it finds. 
//          If no match is found, the function returns VFW_E_NOT_FOUND.
//          The templated version takes an ATL smart pointer and deduces the IID.
///////////////////////////////////////////////////////////////////////

inline HRESULT FindGraphInterface(
    IGraphBuilder *pGraph, 
    REFGUID iid, 
    void **ppUnk)
{
    if (!pGraph || !ppUnk) 
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    bool bFound = false;

    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;

    CHECK_HR(hr = pGraph->EnumFilters(&pEnum));

    // Loop through every filter in the graph.
    while (S_OK == pEnum->Next(1, &pFilter, 0))
    {
        hr = pFilter->QueryInterface(iid, ppUnk);
        if (FAILED(hr))
        {
            // The filter does not expose the interface, but maybe
            // one of its pins does.
            hr = FindPinInterface(pFilter, iid, ppUnk);
        }

        SAFE_RELEASE(pFilter);
        if (SUCCEEDED(hr))
        {
            bFound = true;
            break;
        }
    }

done:
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pFilter);

    return (bFound ? S_OK : VFW_E_NOT_FOUND);
}

template <class Q>
HRESULT FindGraphInterface(IGraphBuilder *pGraph, Q** pp)
{
    return FindGraphInterface(pGraph, __uuidof(Q), (void**)pp);
}




///////////////////////////////////////////////////////////////////////
// Name: GetConnectedFilter
// Desc: Returns the filter on the other side of a pin. 
//
// ie, Given a pin, get the pin connected to it, and return that pin's filter.
// 
// pPin     Pointer to the pin.
// ppFilter Receives a pointer to the filter.
//
///////////////////////////////////////////////////////////////////////

inline HRESULT GetConnectedFilter(IPin *pPin, IBaseFilter **ppFilter)
{
    if (!pPin || !ppFilter)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IPin *pPeer = NULL;
    PIN_INFO info;

    ZeroMemory(&info, sizeof(info));

    CHECK_HR(hr = pPin->ConnectedTo(&pPeer));
    CHECK_HR(hr = pPeer->QueryPinInfo(&info));

    assert(info.pFilter != NULL);
    if (info.pFilter)
    {
        *ppFilter = info.pFilter;   // Return pointer to caller.
        (*ppFilter)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;  // Pin does not have an owning filter! That's weird! 
    }

done:
    SAFE_RELEASE(pPeer);
    SAFE_RELEASE(info.pFilter);
    return hr;
}

///////////////////////////////////////////////////////////////////////
// Name: GetNextFilter
// Desc: Find a filter's upstream or downstream neighbor. 
//       (Returns the first instance found.)
// 
// If there is no matching filter, the function returns VFW_E_NOT_CONNECTED.
///////////////////////////////////////////////////////////////////////

HRESULT inline GetNextFilter(
    IBaseFilter *pFilter, // Pointer to the starting filter
    GraphDirection Dir,    // Direction to search (upstream or downstream)
    IBaseFilter **ppNext) // Receives a pointer to the next filter.
{

    PIN_DIRECTION PinDirection = (Dir == UPSTREAM ? PINDIR_INPUT : PINDIR_OUTPUT); 

    if (!pFilter || !ppNext) 
    {
        return E_POINTER;
    }

    IPin *pPin = NULL;
    HRESULT hr = FindConnectedPin(pFilter, PinDirection, &pPin);
    if (SUCCEEDED(hr))
    {
        hr = GetConnectedFilter(pPin, ppNext);
    }

    SAFE_RELEASE(pPin);

    return hr;
}


///////////////////////////////////////////////////////////////////////
//
// Name:  RemoveFiltersDownstreamOfPin
//
// Removes any filters downstream from pPin, AND removes the filter that owns pPin.
// (This is a recursive function used by RemoveFiltersDownstream, see below.)
///////////////////////////////////////////////////////////////////////

inline HRESULT RemoveFiltersDownstreamOfPin(IGraphBuilder *pGraph, IPin *pPin)
{
    // If the pin is an output pin, and the pin is connected, then
    //   1. Get the owning filter
    //   2. Call RemoveFiltersDownstream recursively
    //   3. Disconnect the pin
    //   4. Remove the owning filter

    BOOL bIsOutput = FALSE;
    HRESULT hr = S_OK;
    IBaseFilter *pFilter = NULL;

    CHECK_HR(hr = IsPinDirection(pPin, PINDIR_OUTPUT, &bIsOutput));

    if (!bIsOutput)
    {
        // This pin is not an input pin, so there is nothing to do.
        return S_OK;
    }

    // Get the owning filter.
    hr = GetConnectedFilter(pPin, &pFilter);
    if (hr == VFW_E_NOT_CONNECTED)
    {
        hr = S_OK; // This pin was not connected, so there is nothing to do.
    }
    else if (SUCCEEDED(hr))
    {
        // Call recursively to remove the filters downstream from this pin.
        CHECK_HR(hr = RemoveFiltersDownstream(pGraph, pFilter));

        // Now remove the owning filter.
        CHECK_HR(hr = RemoveFilter(pGraph, pFilter));
    }

done:
    SAFE_RELEASE(pFilter);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: RemoveFiltersDownstream
// Desc: Remove a filter from the graph, including all filters 
//       downstream from that filter.
//
// Note: This function removes pFilter from the graph and 
//       removes every filter that is downstream from pFilter.
///////////////////////////////////////////////////////////////////////

inline HRESULT RemoveFiltersDownstream(IGraphBuilder *pGraph, IBaseFilter *pFilter)
{
    if (!pGraph || !pFilter)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    CHECK_HR(hr = pFilter->EnumPins(&pEnum));

    while (S_OK == pEnum->Next(1, &pPin, 0))
    {
        CHECK_HR(hr = RemoveFiltersDownstreamOfPin(pGraph, pPin));
        SAFE_RELEASE(pPin);
   }

done:
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pPin);
    return hr;
}

///////////////////////////////////////////////////////////////////////
//
// Name: RemoveUnconnectedFilters
// Desc: Remove all unconnected filters from the graph.
//
///////////////////////////////////////////////////////////////////////

inline HRESULT RemoveUnconnectedFilters(IGraphBuilder *pGraph)
{
    if (pGraph == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter = NULL;
    IPin *pPin = NULL;

    CHECK_HR(hr = pGraph->EnumFilters(&pEnum));

    // Go through the list of filters in the graph.
    while (S_OK == pEnum->Next(1, &pFilter, NULL))
    {
        // Find a connected pin on this filter.
        HRESULT hr2 = FindMatchingPin(pFilter, MatchPinConnection(TRUE), &pPin);
        if (SUCCEEDED(hr2))
        {
            // If it's connected, don't remove the filter.
            SAFE_RELEASE(pPin);
            continue;
        }
        assert(pPin == NULL);
        CHECK_HR(hr = RemoveFilter(pGraph, pFilter));

        // The previous call made the enumerator stale. 
        pEnum->Reset(); 
    }

done:
    SAFE_RELEASE(pEnum);
    SAFE_RELEASE(pFilter);
    SAFE_RELEASE(pPin);
    return hr;
}

/**************************************************************************

    Misc. Helper Functions

**************************************************************************/

// CopyFormatBlock: 
// Allocates memory for the format block in the media type and copies the 
// buffer into the format block. Also releases the previous format block.
inline HRESULT CopyFormatBlock(AM_MEDIA_TYPE *pmt, const BYTE *pFormat, DWORD cbSize)
{
    if (pmt == NULL)
    {
        return E_POINTER;
    }

    if (cbSize == 0)
    {
        _FreeMediaType(*pmt);
        return S_OK;
    }

    if (pFormat == NULL)
    {
        return E_INVALIDARG;
    }

    // Reallocate the format block.
    // Note: It is valid to pass NULL to CoTaskMemRealloc.
    // Note: If CoTaskMemRealloc fails to allocate a new block, it does
    //       free the old block. 
    BYTE *pbFormat = (BYTE*)CoTaskMemRealloc(pmt->pbFormat, cbSize);  
    if (pbFormat == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pmt->pbFormat = pbFormat;
    pmt->cbFormat = cbSize;
    CopyMemory(pmt->pbFormat, pFormat, cbSize);
    return S_OK;
}



// RectWidth: Returns the width of a rectangle.
inline LONG RectWidth(const RECT& rc) { return rc.right - rc.left; }

// RectHeight: Returns the height of a rectangle.
inline LONG RectHeight(const RECT& rc) { return rc.bottom - rc.top; }


/********************* Time conversion functions *********************/

///////////////////////////////////////////////////////////////////////
// FramesPerSecToFrameLength
// Converts from frames-to-second to frame duration.
///////////////////////////////////////////////////////////////////////

inline REFERENCE_TIME FramesPerSecToFrameLength(double fps) 
{ 
    return (REFERENCE_TIME)((double)ONE_SECOND / fps);
}

///////////////////////////////////////////////////////////////////////
// RefTimeToMsec
// Convert REFERENCE_TIME units to milliseconds (taken from CRefTime)
///////////////////////////////////////////////////////////////////////

inline LONG RefTimeToMsec(const REFERENCE_TIME& time)
{
  return (LONG)(time / (ONE_SECOND / ONE_MSEC));
}

///////////////////////////////////////////////////////////////////////
// RefTimeToSeconds
// Converts reference time (100 nanosecond units) to floating-point seconds.
///////////////////////////////////////////////////////////////////////

inline double RefTimeToSeconds(const REFERENCE_TIME& rt)
{
    return double(rt) / double(ONE_SECOND);
}

///////////////////////////////////////////////////////////////////////
// SecondsToRefTime
// Converts seconds to reference time.
///////////////////////////////////////////////////////////////////////

inline REFERENCE_TIME SecondsToRefTime(const double& sec)
{
    return (REFERENCE_TIME)(sec * double(ONE_SECOND));
}

///////////////////////////////////////////////////////////////////////
// MsecToRefTime
// Converts milliseconds to reference time.
///////////////////////////////////////////////////////////////////////

inline REFERENCE_TIME MsecToRefTime(const LONG& msec)
{
    return (REFERENCE_TIME)(msec * ONE_MSEC);
}


/********************* GraphEdit functions *********************/


///////////////////////////////////////////////////////////////////////
// Name: SaveGraphFile
// Desc: Save a filter graph to a .grf (GraphEdit) file.
///////////////////////////////////////////////////////////////////////

inline HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath) 
{
    if (pGraph == NULL)
    {
        return E_POINTER;
    }

    const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
    
    HRESULT hr = S_OK;
    IStorage *pStorage = NULL;
    IStream *pStream = NULL;
    IPersistStream *pPersist = NULL;

    CHECK_HR(hr = StgCreateDocfile(
        wszPath,
        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, &pStorage));

    CHECK_HR(hr = pStorage->CreateStream(
            wszStreamName,
            STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
            0, 
            0, 
            &pStream));
    CHECK_HR(hr = pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist));

    CHECK_HR(hr = pPersist->Save(pStream, TRUE));

    CHECK_HR(hr = pStorage->Commit(STGC_DEFAULT));

done:
    SAFE_RELEASE(pStorage);
    SAFE_RELEASE(pStream);
    SAFE_RELEASE(pPersist);
    return hr;
}

///////////////////////////////////////////////////////////////////////
// Name: LoadGraphFile
// Desc: Load a .grf (GraphEdit) file.
///////////////////////////////////////////////////////////////////////

inline HRESULT LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName)
{
    if (pGraph == NULL)
    {
        return E_POINTER;
    }

    if (wszName == NULL)
    {
        return E_POINTER;
    }

    if (S_OK != StgIsStorageFile(wszName))
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    IStorage *pStorage = NULL;
    IPersistStream *pPersistStream = NULL;
    IStream *pStream = NULL;

    CHECK_HR(hr = StgOpenStorage(
        wszName, 
        0, 
        STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 
        0, 
        0, 
        &pStorage));

    CHECK_HR(hr = pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersistStream));

    CHECK_HR(hr = pStorage->OpenStream(L"ActiveMovieGraph", 0, 
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream));

    CHECK_HR(hr = pPersistStream->Load(pStream));

done:
    SAFE_RELEASE(pStorage);
    SAFE_RELEASE(pPersistStream);
    SAFE_RELEASE(pStream);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: AddGraphToRot
// Desc: Adds a DirectShow filter graph to the Running Object Table,
//       allowing GraphEdit to load a remote filter graph if enabled.
//
// pUnkGraph: Pointer to the Filter Graph Manager
// pdwRegister: Receives a token. Pass this value to RemoveFromRot.
///////////////////////////////////////////////////////////////////////

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to load a remote filter graph.
inline HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
    IMoniker * pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;
    WCHAR wsz[128];
    HRESULT hr = S_OK;

    if (!pUnkGraph || !pdwRegister)
    {
        return E_POINTER;
    }

    CHECK_HR(hr = GetRunningObjectTable(0, &pROT));

    // Format the string for the registration
    CHECK_HR(hr = StringCchPrintfW(wsz, NUMELMS(wsz), L"FilterGraph %08x pid %08x\0", 
        (DWORD_PTR)pUnkGraph, GetCurrentProcessId()));

    // Create the moniker
    CHECK_HR(hr = CreateItemMoniker(L"!", wsz, &pMoniker));

    // Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
    // to the object.  Using this flag will cause the object to remain
    // registered until it is explicitly revoked with the Revoke() method.
    //
    // Not using this flag means that if GraphEdit remotely connects
    // to this graph and then GraphEdit exits, this object registration 
    // will be deleted, causing future attempts by GraphEdit to fail until
    // this application is restarted or until the graph is registered again.
    CHECK_HR(hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
                        pMoniker, pdwRegister));

done:
    SAFE_RELEASE(pMoniker);
    SAFE_RELEASE(pROT);
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Name: RemoveGraphFromRot
// Desc: Removes a DirectShow filter graph from the Running Object Table.
//
// dwRegister: Token returned by AddGraphToRot function.
///////////////////////////////////////////////////////////////////////

inline void RemoveGraphFromRot(DWORD dwRegister)
{
    IRunningObjectTable *pROT = NULL;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
    {
        pROT->Revoke(dwRegister);
    }

    SAFE_RELEASE(pROT);
}


/********************* Audio and Video Format Functions *********************/


///////////////////////////////////////////////////////////////////////
// Name: CreatePCMAudioType
// Desc: Initialize a PCM audio type with a WAVEFORMATEX format block.
//       (This function does not handle WAVEFORMATEXTENSIBLE formats.)
//
// If the method succeeds, call FreeMediaType to free the format block.
///////////////////////////////////////////////////////////////////////

inline HRESULT CreatePCMAudioType(
    AM_MEDIA_TYPE& mt,      // Media type to populate
    WORD nChannels,         // Number of channels
    DWORD nSamplesPerSec,   // Samples per second
    WORD wBitsPerSample     // Bits per sample
    )
{
    _FreeMediaType(mt);


    mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (!mt.pbFormat)
    {
        return E_OUTOFMEMORY;
    }
    mt.cbFormat = sizeof(WAVEFORMATEX);

    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.formattype = FORMAT_WaveFormatEx;

    WAVEFORMATEX *pWav = (WAVEFORMATEX*)mt.pbFormat;
    pWav->wFormatTag = WAVE_FORMAT_PCM;
    pWav->nChannels  = nChannels;
    pWav->nSamplesPerSec = nSamplesPerSec;
    pWav->wBitsPerSample = wBitsPerSample;
    pWav->cbSize = 0;

    // Derived values
    pWav->nBlockAlign = nChannels * (wBitsPerSample  / 8);
    pWav->nAvgBytesPerSec = nSamplesPerSec * pWav->nBlockAlign;

    return S_OK;
}



///////////////////////////////////////////////////////////////////////
// Name: CreateRGBVideoType
// Desc: Initialize an uncompressed RGB video media type.
//       (Allocates the palette table for palettized video)
//
// mt:         Media type to populate
// iBitDepth:  Bits per pixel. Must be 1, 4, 8, 16, 24, or 32
// Width:      Width in pixels
// Height:     Height in pixels. Use > 0 for bottom-up DIBs, < 0 for top-down DIB
// fps:        Frame rate, in frames per second
//
// If the method succeeds, call FreeMediaType to free the format block.
///////////////////////////////////////////////////////////////////////

inline HRESULT CreateRGBVideoType(AM_MEDIA_TYPE &mt, WORD iBitDepth, long Width, long Height, 
                           double fps)
{
    DWORD color_mask_565[] = { 0x00F800, 0x0007E0, 0x00001F };


    if ((iBitDepth != 1) && (iBitDepth != 4) && (iBitDepth != 8) &&
        (iBitDepth != 16) && (iBitDepth != 24) && (iBitDepth != 32))
    {
        return E_INVALIDARG;
    }

    if (Width < 0)
    {
        return E_INVALIDARG;
    }

    _FreeMediaType(mt);

    mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFO));
    if (!mt.pbFormat)
    {
        return E_OUTOFMEMORY;
    }
    mt.cbFormat = sizeof(VIDEOINFO);


    VIDEOINFO *pvi = (VIDEOINFO*)mt.pbFormat;
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    pvi->AvgTimePerFrame = FramesPerSecToFrameLength(fps);

    BITMAPINFOHEADER *pBmi = &(pvi->bmiHeader);
    pBmi->biSize = sizeof(BITMAPINFOHEADER);
    pBmi->biWidth = Width;
    pBmi->biHeight = Height;
    pBmi->biPlanes = 1;
    pBmi->biBitCount = iBitDepth;

    if (iBitDepth == 16)
    {
        pBmi->biCompression = BI_BITFIELDS;
        memcpy(pvi->dwBitMasks, color_mask_565, sizeof(DWORD) * 3);
    }
    else
    {
        pBmi->biCompression = BI_RGB;
    }

    if (iBitDepth <= 8)
    {
        // Palettized format.
        pBmi->biClrUsed = PALETTE_ENTRIES(pvi);

        HDC hdc = GetDC(NULL);  // hdc for the current display.
        if (hdc == NULL)
        {
            _FreeMediaType(mt);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        GetSystemPaletteEntries(hdc, 0, pBmi->biClrUsed, (PALETTEENTRY*)pvi->bmiColors);

        ReleaseDC(NULL, hdc);
    }

    pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);

    mt.majortype == MEDIATYPE_Video;
    mt.subtype = FORMAT_VideoInfo;

    switch (iBitDepth)
    {
    case 1:
        mt.subtype = MEDIASUBTYPE_RGB1;
        break;
    case 4:
        mt.subtype = MEDIASUBTYPE_RGB4;
        break;
    case 8:
        mt.subtype = MEDIASUBTYPE_RGB8;
        break;
    case 16:
        mt.subtype = MEDIASUBTYPE_RGB565;
        break;
    case 24:
        mt.subtype = MEDIASUBTYPE_RGB24;
        break;
    case 32:
        mt.subtype = MEDIASUBTYPE_RGB32;
    }

    mt.lSampleSize = pvi->bmiHeader.biSizeImage;
    mt.bTemporalCompression = FALSE;
    mt.bFixedSizeSamples = TRUE;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////
// Name: LetterBoxRect
// Desc: Find the largest rectangle that fits inside rcDest and has
//       the specified aspect ratio.
// 
// aspectRatio: Desired aspect ratio
// rcDest:      Destination rectangle (defines the bounds)
// prcResult:   Pointer to a RECT struct. The method fills in the
//              struct with the letterboxed rectangle.
//
///////////////////////////////////////////////////////////////////////

inline HRESULT LetterBoxRect(const SIZE &aspectRatio, const RECT &rcDest, RECT *prcResult)
{
    if (prcResult == NULL)
    {
        return E_POINTER;
    }

    // Avoid divide by zero (even though MulDiv handles this)
    if (aspectRatio.cx == 0 || aspectRatio.cy == 0)
    {
        return E_INVALIDARG;
    }

    LONG width, height;

    LONG SrcWidth = aspectRatio.cx;
    LONG SrcHeight = aspectRatio.cy;
    LONG DestWidth = rcDest.right - rcDest.left;
    LONG DestHeight = rcDest.bottom - rcDest.top;


   // First try: Letterbox along the sides. ("pillarbox")
    width = MulDiv(DestHeight, SrcWidth, SrcHeight);
    height = DestHeight;
    if (width > DestWidth)
    {
        // Letterbox along the top and bottom.
        width = DestWidth;
        height = MulDiv(DestWidth, SrcHeight, SrcWidth);
    }

    if (width == -1 || height == -1)
    {
        // MulDiv caught an overflow or divide by zero)
        return E_FAIL;
    }

    assert(width <= DestWidth);
    assert(height <= DestHeight);

    // Fill in the rectangle
    prcResult->left = rcDest.left + ((DestWidth - width) / 2);
    prcResult->right = prcResult->left + width;
    prcResult->top = rcDest.top + ((DestHeight - height) / 2);
    prcResult->bottom = prcResult->top + height;

    return S_OK;
}


/********************* Media type functions *********************/


//----------------------------------------------------------------------------
// SetMediaTypeFormat: Sets the format block of a media type
// 
// pmt: Pointer to the AM_MEDIA_TYPE structure. Cannot be NULL
// pBuffer: Pointer to the format block. (Can be NULL)
// cbBuffer: Size of pBuffer in bytes
//
// This function clears the old format block and copies the new
// format block into the media type structure.
//----------------------------------------------------------------------------

inline HRESULT SetMediaTypeFormatBlock(AM_MEDIA_TYPE *pmt, BYTE *pBuffer, DWORD cbBuffer)
{
  if (!pmt)
  {
    return E_POINTER;
  }
  if (!pBuffer && cbBuffer > 0)
  {
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;

  pmt->pbFormat = (BYTE*)CoTaskMemRealloc(pmt->pbFormat, cbBuffer);
  pmt->cbFormat = cbBuffer;

  if (cbBuffer > 0)
  {
    if (pmt->pbFormat)
    {
      CopyMemory(pmt->pbFormat, pBuffer, cbBuffer);
    }
    else
    {
      pmt->cbFormat = 0;
      hr = E_OUTOFMEMORY;  // CoTaskMemRealloc failed
    }
  }    

  return hr;
}



// The following functions are defined in the DirectShow base class library.
// They are redefined here for convenience, because many applications do not
// need to link to the base class library.

// FreeMediaType: Release the format block for a media type.
inline void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // Unecessary because pUnk should not be used, but safest.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

// DeleteMediaType:
// Delete a media type structure that was created on the heap, including the
// format block)
inline void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != NULL)
    {
        _FreeMediaType(*pmt); 
        CoTaskMemFree(pmt);
    }
}


#ifndef __STREAMS__
#define FreeMediaType _FreeMediaType
#define DeleteMediaType _DeleteMediaType
#endif



#ifdef __IVMRWindowlessControl9_FWD_DEFINED__

// AddVMR9Filter
inline HRESULT AddVMR9Filter(
    IGraphBuilder *pGraph,  // Pointer to the Filter Graph Manager
    HWND hwndVideo,         // Handle to the application's video window.
    DWORD cMaxStreams,      // Maximum number of video streams.
    IVMRWindowlessControl9 **ppWindowless   // Optional. 
    )
{
    if (pGraph == NULL)
    {
        return E_POINTER;
    }
    if (hwndVideo == NULL)
    {
        return E_INVALIDARG;
    }
    if (cMaxStreams < 1)
    {
        return E_INVALIDARG;
    }
    // ppWindowless can be NULL

    HRESULT hr = S_OK;

    IBaseFilter *pVMR = NULL;
    IVMRFilterConfig9 *pConfig = NULL;;
    IVMRWindowlessControl9 *pWindowless = NULL;

    CHECK_HR(hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer9, &pVMR, L"Video Mixing Renderer 9"));

    CHECK_HR(hr = pVMR->QueryInterface(IID_IVMRFilterConfig9, (LPVOID *)&pConfig));

    CHECK_HR(hr = pConfig->SetNumberOfStreams(cMaxStreams));

    CHECK_HR(hr = pConfig->SetRenderingMode(VMR9Mode_Windowless));

    CHECK_HR(hr = pVMR->QueryInterface(IID_IVMRWindowlessControl9, (LPVOID *)&pWindowless));

    CHECK_HR(hr = pWindowless->SetVideoClippingWindow(hwndVideo));

    CHECK_HR(hr = pWindowless->SetAspectRatioMode(VMR9ARMode_LetterBox));

    if (ppWindowless)
    {
        *ppWindowless = pWindowless;
        (*ppWindowless)->AddRef();
    }

done:
    SAFE_RELEASE(pVMR);
    SAFE_RELEASE(pConfig);
    SAFE_RELEASE(pWindowless);
    return hr;
}

#endif // __IVMRWindowlessControl9_FWD_DEFINED__


// RenderFileToVideoRenderer:
// Renders a media file to an existing video renderer in the graph. 
// NOTE: The caller must add the video renderer to the graph before calling this method.
inline HRESULT RenderFileToVideoRenderer(IGraphBuilder *pGraph, WCHAR *wFileName, BOOL bRenderAudio = TRUE)
{
    if (pGraph == NULL || wFileName == NULL)
    {
        return E_POINTER;
    }

  HRESULT hr = S_OK;

  BOOL bRenderedAnyPin = FALSE;

    IBaseFilter *pSource = NULL;
    IPin *pPin = NULL;
  IFilterGraph2 *pGraph2 = NULL;
  IEnumPins *pEnum = NULL;
  IBaseFilter *pAudioRenderer = NULL;

    // Get the IFilterGraph2 interface from the Filter Graph Manager.
    CHECK_HR(hr = pGraph->QueryInterface(IID_IFilterGraph2, (void**)&pGraph2));

    // Add the source filter.
    CHECK_HR(hr = pGraph->AddSourceFilter(wFileName, L"SOURCE", &pSource));

    // Add the DSound Renderer to the graph.
    if (bRenderAudio)
    {
        CHECK_HR(hr = AddFilterByCLSID(pGraph, CLSID_DSoundRender, &pAudioRenderer, L"Audio Renderer"));
    }

    // Enumerate the pins on the source filter.
    CHECK_HR(hr = pSource->EnumPins(&pEnum));

  // Loop through all the pins

  while (S_OK == pEnum->Next(1, &pPin, NULL))
  {      
    // Try to render this pin. 
    // It's OK if we fail some pins, if at least one pin renders.
    HRESULT hr2 = pGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

    pPin->Release();

    if (SUCCEEDED(hr2))
    {
      bRenderedAnyPin = TRUE;
    }
  }

    // Try to remove any unconnected filters.
  CHECK_HR(hr = RemoveUnconnectedFilters(pGraph));

done:
  SAFE_RELEASE(pEnum);
  SAFE_RELEASE(pAudioRenderer);
  SAFE_RELEASE(pGraph2);

  // If we succeeded to this point, make sure we rendered at least one 
  // stream.
  if (SUCCEEDED(hr))
  {
    if (!bRenderedAnyPin)
    {
      hr = VFW_E_CANNOT_RENDER;
    }
  }

  return hr;
}
