/*
 *
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
#ifndef _DSTEMPLATE_H
#define _DSTEMPLATE_H

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include <map>
#include "streams.h"
#include "utils\stdstring.h"

namespace Com
{


  typedef std::vector<CMediaType>	MediaTypes;

  //-------------------------------------------------------------------------
  //
  //	URI class
  //
  //-------------------------------------------------------------------------

  class URI
  {
  public:
    CStdString		protocol;
    CStdString		host;
    CStdString		request_url;
    CStdString		complete_request;
    int			port;

  public:
    URI();
    URI(const URI &u);
    URI(CStdString url);
    virtual ~URI();

    URI &operator =(const URI &u);
    URI &operator =(CStdString url);

    int Parse(CStdString url);
  public:
    operator CStdString() { return complete_request; }
  };

  //-------------------------------------------------------------------------
  //
  //	Helper Classes
  //
  //-------------------------------------------------------------------------

  class PinTemplate
  {
  public:
    PIN_DIRECTION	dir;
    BOOL			rendered;
    BOOL			many;
    int				types;
    std::vector<GUID>	major;
    std::vector<GUID>	minor;

  public:
    PinTemplate();
    PinTemplate(const PinTemplate &pt);
    virtual ~PinTemplate();
    PinTemplate &operator =(const PinTemplate &pt);
  };

  class FilterTemplate
  {
  public:
    CStdString		name;
    CStdString		moniker_name;
    GUID		clsid;
    GUID		category;
    DWORD		version;
    DWORD		merit;
    IMoniker	*moniker;
    CStdString		file;
    bool		file_exists;

    std::vector<PinTemplate>		input_pins;
    std::vector<PinTemplate>		output_pins;

    enum {
      FT_FILTER = 0,
      FT_DMO = 1,
      FT_KSPROXY = 2,
      FT_ACM_ICM = 3,
      FT_PNP = 4
    };
    int			type;

  public:
    FilterTemplate();
    FilterTemplate(const FilterTemplate &ft);
    virtual ~FilterTemplate();
    FilterTemplate &operator =(const FilterTemplate &ft);

    HRESULT CreateInstance(IBaseFilter **filter);
    HRESULT FindFilename();

    int LoadFromMoniker(CStdString displayname);
    int Load(char *buf, int size);
    int ParseMonikerName();
  };

  class FilterCategory
  {
  public:
    CStdString		name;
    GUID		clsid;
    bool		is_dmo;				// is this category DMO ?
  public:
    FilterCategory();
    FilterCategory(CStdString nm, GUID cat_clsid, bool dmo = false);
    FilterCategory(const FilterCategory &fc);
    virtual ~FilterCategory();
    FilterCategory &operator =(const FilterCategory &fc);
  };

  class FilterCategories
  {
  public:
    std::vector<FilterCategory>	categories;
  public:
    FilterCategories();
    virtual ~FilterCategories();

    int Enumerate();
  };


  class FilterTemplates
  {
  public:
    std::vector<FilterTemplate>	filters;
  public:
    FilterTemplates();
    virtual ~FilterTemplates();

    int Enumerate(FilterCategory &cat);
    int Enumerate(GUID clsid);
    int EnumerateDMO(GUID clsid);

    int EnumerateCompatible(MediaTypes &mtypes, DWORD min_merit, bool need_output, bool exact);

    int EnumerateAudioRenderers();
    int EnumerateVideoRenderers();

    int Find(CStdString name, FilterTemplate *filter);
    int Find(GUID clsid, FilterTemplate *filter);
    int AddFilters(IEnumMoniker *emoniker, int enumtype = 0, GUID category = GUID_NULL);

    // testing
    int IsVideoRenderer(FilterTemplate &filter);

    void SortByName();
    void SwapItems(int i, int j);
    void _Sort_(int lo, int hi);

    // vytvaranie
    HRESULT CreateInstance(CStdString name, IBaseFilter **filter);
    HRESULT CreateInstance(GUID clsid, IBaseFilter **filter);
  };

  class Pin
  {
  public:
    IBaseFilter		*filter;
    IPin			*pin;
    CStdString			name;
    PIN_DIRECTION	dir;

    enum {
      PIN_FLAG_INPUT = 1,
      PIN_FLAG_OUTPUT = 2,
      PIN_FLAG_CONNECTED = 4,
      PIN_FLAG_NOT_CONNECTED = 8,
      PIN_FLAG_ALL = 0xffff
    };
  public:
    Pin();
    Pin(const Pin &p);
    virtual ~Pin();
    Pin &operator =(const Pin &p);
  };

  typedef std::vector<Pin>			PinArray;

  HRESULT DisplayPropertyPage(IBaseFilter *filter, HWND parent = NULL);

  HRESULT EnumPins(IBaseFilter *filter, PinArray &pins, int flags);
  HRESULT EnumMediaTypes(IPin *pin, MediaTypes &types);
  namespace Monogram
  {
    HRESULT ConnectFilters(IGraphBuilder *gb, IBaseFilter *output, IBaseFilter *input, bool direct = false);
  }
  HRESULT ConnectPin(IGraphBuilder *gb, IPin *output, IBaseFilter *input, bool direct = false);

  bool IsVideoUncompressed(GUID subtype);

  CStdString get_next_token(CStdString &str, CStdString separator);

  HRESULT UnregisterFilter(GUID clsid, GUID category);
  HRESULT UnregisterCOM(GUID clsid);

};
#endif