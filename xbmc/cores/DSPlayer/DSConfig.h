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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DSPropertyPage.h"
#include "DSGraph.h"
#include "DSUtil/SmartPtr.h"

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
  virtual CStdString GetDXVAMode();
  void SetDXVAGuid(const GUID& dxvaguid);

  /**
   * Get a list of filters with a property page
   * @return The list of filters with a property page
   */
  virtual std::vector<IBaseFilter *> GetFiltersWithPropertyPages(void) { return m_pPropertiesFilters; };
  /**
   * Show the property page for the filter
   * @param[in] pBF Filter whose showing property page
   */
  void ShowPropertyPage(IBaseFilter *pBF);
protected:
  /**
   * If the filter expose a property page, add it to m_pPropertiesFilters
   * @param[in] pBF The filter to test
   * @return True if the filter expose a property page, false else*/
  bool LoadPropertiesPage(IBaseFilter *pBF);
private:
  void CreatePropertiesXml();
  CCriticalSection m_pLock;
  CStdString                     m_pStrDXVA;
  //Direct Show Filters
  std::vector<IBaseFilter *>     m_pPropertiesFilters;
  //current page
  CDSPropertyPage*               m_pCurrentProperty;
};

extern class CDSConfig g_dsconfig;