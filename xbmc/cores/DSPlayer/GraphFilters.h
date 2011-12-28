#pragma once
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#include "streams.h"

/// Informations about a filter
struct SFilterInfos
{
  SFilterInfos()
  {
    Clear();
  }
  
  void Clear()
  {
    pBF = NULL;
    osdname = "";
    guid = GUID_NULL;
    isinternal = false;
    pData = NULL;
  }

  Com::SmartPtr<IBaseFilter> pBF; ///< Pointer to the IBaseFilter interface. May be NULL
  CStdString osdname; ///< OSD Name of the filter
  GUID guid; ///< GUID of the filter
  bool isinternal; ///<  Releasing is not done the same way for internal filters
  void *pData; ///< If the filter is internal, there may be some additionnal data
};

/// Specific informations about the video renderer filter
struct SVideoRendererFilterInfos: SFilterInfos
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

enum DIRECTSHOW_RENDERER
{
    DIRECTSHOW_RENDERER_VMR9 = 1,
    DIRECTSHOW_RENDERER_EVR = 2,
    DIRECTSHOW_RENDERER_UNDEF = 3
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
  void SetIsDVD(bool val) {  m_isDVD = val; }
  void SetCurrentRenderer(DIRECTSHOW_RENDERER renderer) { m_CurrentRenderer = renderer; }

private:
  CGraphFilters();
  ~CGraphFilters();

  static CGraphFilters* m_pSingleton;

  bool m_isDVD;
  bool m_UsingDXVADecoder;
  DIRECTSHOW_RENDERER m_CurrentRenderer;
};