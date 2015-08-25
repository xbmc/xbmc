#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#include "IPaintCallback.h"
#include "streams.h"
#include "utils/CharsetConverter.h"
#include "system.h"

/// Informations about a filter
struct SFilterInfos
{
  SFilterInfos()
  {
    Clear();
  }

  ~SFilterInfos()
  {
    if (isinternal != true)
      SAFE_DELETE(pData);
  }

  void Clear()
  {
    pBF = NULL;
    osdname = "";
    guid = GUID_NULL;
    isinternal = false;
    internalLav = false;
    pData = NULL;
  }

  void SetFilterInfo(IBaseFilter * pBF)
  {
    CStdString filterName;
    g_charsetConverter.wToUTF8(GetFilterName(pBF), filterName);
    osdname = filterName;
    guid = GetCLSID(pBF);
  }

  Com::SmartPtr<IBaseFilter> pBF; ///< Pointer to the IBaseFilter interface. May be NULL
  CStdString osdname; ///< OSD Name of the filter
  GUID guid; ///< GUID of the filter
  bool isinternal; ///<  Releasing is not done the same way for internal filters
  bool internalLav = false;
  void *pData; ///< If the filter is internal, there may be some additionnal data
};

/// Specific informations about the video renderer filter
struct SVideoRendererFilterInfos : SFilterInfos
{
  SVideoRendererFilterInfos()
    : SFilterInfos()
  {
    Clear();
  }

  void Clear()
  {
    pQualProp = NULL;
    __super::Clear();
  }
  Com::SmartPtr<IQualProp> pQualProp; ///< Pointer to IQualProp interface. May be NULL if the video renderer doesn't implement IQualProp
};

/// Informations about DVD filters
struct SDVDFilters
{
  SDVDFilters()
  {
    Clear();
  }

  void Clear()
  {
    dvdControl.Release();
    dvdInfo.Release();
  }

  Com::SmartQIPtr<IDvdControl2> dvdControl; ///< Pointer to IDvdControl2 interface. May be NULL
  Com::SmartQIPtr<IDvdInfo2> dvdInfo; ///< Pointer to IDvdInfo2 interface. May be NULL
};

enum LAVFILTERS_TYPE
{
  LAVSPLITTER,
  LAVVIDEO,
  LAVAUDIO,
  XYSUBFILTER,
  NULLFILTER
};

enum FILTERSMAN_TYPE
{
  INTERNALFILTERS,
  MEDIARULES,
  DSMERITS,
};


/*MADVR*/
enum DIRECTSHOW_RENDERER
{
  DIRECTSHOW_RENDERER_VMR9 = 1,
  DIRECTSHOW_RENDERER_EVR = 2,
  DIRECTSHOW_RENDERER_MADVR = 3,
  DIRECTSHOW_RENDERER_UNDEF = 4
};

/** @brief Centralize graph filters management

  Our graph can contains Sources, Splitters, Audio renderers, Audio decoders, Video decoders and Extras filters. This singleton class centralize all the data related to these filters.
  */
class CGraphFilters
{
public:
  /// Retrieve singleton instance
  static CGraphFilters* Get();
  /// Destroy singleton instance
  static void Destroy()
  {
    delete m_pSingleton;
    m_pSingleton = NULL;
  }

  /**
   * Informations about the source filter
   * @note It may no have a source filter in the graph, because splitters are also source filters. A source filter is only needed when playing from internet, RAR, ... but not for media file
   **/
  SFilterInfos Source;
  ///Informations about the splitter filter
  SFilterInfos Splitter;
  ///Informations about the video decoder
  SFilterInfos Video;
  ///Informations about the audio decoder
  SFilterInfos Audio;
  ///Informations about the video decoder
  SFilterInfos Subs;
  ///Informations about the audio renderer
  SFilterInfos AudioRenderer;
  ///Informations about the video renderer
  SVideoRendererFilterInfos VideoRenderer;
  ///Informations about extras filters
  std::vector<SFilterInfos> Extras;
  /**
   * Informations about the DVD filters
   * @note The structure is not filled is the current media file isn't a DVD
   */
  SDVDFilters DVD;

  /// @return The current renderer type (EVR or VMR9)
  DIRECTSHOW_RENDERER GetCurrentRenderer() { return m_CurrentRenderer; }

  /// @return True if using DXVA, false otherwise
  bool IsUsingDXVADecoder() { return m_UsingDXVADecoder; }

  /// @return True if we are playing a DVD, false otherwise
  bool IsDVD() { return m_isDVD; }

  void SetIsUsingDXVADecoder(bool val) { m_UsingDXVADecoder = val; }
  void SetIsDVD(bool val) { m_isDVD = val; }
  void SetCurrentRenderer(DIRECTSHOW_RENDERER renderer) { m_CurrentRenderer = renderer; }

  bool HasSubFilter() { return m_hsubfilter; }
  void SetHasSubFilter(bool b) { m_hsubfilter = b; }
  void SetKodiRealFS(bool b) { m_isKodiRealFS = b; }
  void ShowLavFiltersPage(LAVFILTERS_TYPE type, bool showPropertyPage);
  void SetupLavSettings(LAVFILTERS_TYPE type, IBaseFilter *pBF);
  bool SetLavInternal(LAVFILTERS_TYPE type, IBaseFilter *pBF);
  bool GetLavSettings(LAVFILTERS_TYPE type, IBaseFilter *pBF);
  bool SetLavSettings(LAVFILTERS_TYPE type, IBaseFilter *pBF);
  bool LoadLavSettings(LAVFILTERS_TYPE type );
  bool SaveLavSettings(LAVFILTERS_TYPE type );
  bool IsInternalFilter(IBaseFilter *pBF);
  bool IsRegisteredXYSubFilter();
  bool UsingMediaPortalTsReader() 
  { 
    return ((Splitter.guid != GUID_NULL) && !(StringFromGUID(Splitter.guid).compare(L"{B9559486-E1BB-45D3-A2A2-9A7AFE49B23F}"))); 
  }
  
private:
  CGraphFilters();
  ~CGraphFilters();

  static CGraphFilters* m_pSingleton;

  bool m_isKodiRealFS;
  bool m_hsubfilter;
  bool m_isDVD;
  bool m_UsingDXVADecoder;
  DIRECTSHOW_RENDERER m_CurrentRenderer;
};
