#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "StdString.h"
#include <streams.h>
#include <map>

#include "igraphbuilder2.h"
#include "Filters/IMpaDecFilter.h"
#include "Filters/IMPCVideoDecFilter.h"
#include "Filters/IffDecoder.h"
#include "Filters/IffdshowBase.h"
#include "Filters/IffdshowDecVideo.h"
#include "DSPropertyPage.h"
#include "DSGraph.h"
#include "DShowUtil/smartptr.h"

class CDSGraph;
class CDSPropertyPage;

/**
 * Configure filters
 */
class CDSConfig
{
public:
  /// Constructor
  CDSConfig(void);
  /// Destructor
  virtual ~CDSConfig(void);

  /**
   * Clear every interfaces
   */
  void ClearConfig();
  /**
   * Configure the filters in the graph
   * @param[in] pGB Pointer to the graph interface
   * @return A HRESULT code
   */
  virtual HRESULT ConfigureFilters();
  /**
   * Retrieve a string containing the current DXVA mode
   * @return A CStdString containing the current DXVA mode
   */
  CStdString GetDXVAMode(void);

  /**
   * Get a list of filters with a property page
   * @return The list of filters with a property page
   */
  virtual std::vector<IBaseFilter *> GetFiltersWithPropertyPages(void) { return m_pPropertiesFilters; };
  /**
   * Show the property page for the filter
   * @param[in] pBF Filter whose showing prroperty page
   */
  void ShowPropertyPage(IBaseFilter *pBF);
  
  bool LoadffdshowSubtitles(CStdString filePath);

  /// Pointer to a CDSGraph instance
  CDSGraph *          pGraph;
  /** @defgroup FFDShow FFDShow related interface
   * Variable related to the FFDShow filter
   * @remarks All variables are NULL if the current filter isn't FFDShow
   * @{ */
  /// Pointer to a IffdshowBaseA interface
  //IffdshowBaseA*      pIffdshowBase;
  /// Pointer to a IffdshowDecVideoA interface
  //IffdshowDecVideoA*  pIffdshowDecFilter;
  /// Pointer to a IffDecoder interface
  IffDecoder*         pIffdshowDecoder;
  /** @} */
  //Com::SmartQIPtr<IQualProp, &IID_IQualProp> pQualProp;
protected:
  /**
   * If the filter expose a property page, add it to m_pPropertiesFilters
   * @param[in] pBF The filter to test
   * @return True if the filter expose a property page, false else*/
  bool LoadPropertiesPage(IBaseFilter *pBF);
  /**
   * Load configuration from the MP Audio Decoder
   * @param[in] pBF Try to load the configuration from this filter
   * @return True if the filter is a MP Audio Decoder, false else
   */
  bool GetMpaDec(IBaseFilter* pBF);
  /**
   * Load configuration from FFDShow
   * @param[in] pBF Try to load the configuration from this filter
   * @return True if the filter is FFDShow, false else
   */
  bool GetffdshowFilters(IBaseFilter* pBF);
  
private:
  void CreatePropertiesXml();
  CCritSec m_pLock;

  //Direct Show Filters

  Com::SmartPtr<IMpaDecFilter>         m_pIMpaDecFilter;
  std::vector<IBaseFilter *>     m_pPropertiesFilters;
  //current page
  CDSPropertyPage*               m_pCurrentProperty;
};

extern class CDSConfig g_dsconfig;