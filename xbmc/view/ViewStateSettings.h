#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>

#include "ViewState.h"
#include "guilib/GraphicContext.h"
#include "settings/ISubSettings.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CViewStateSettings : public ISubSettings
{
public:
  static CViewStateSettings& Get();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  const CViewState* Get(const std::string &viewState) const;
  CViewState* Get(const std::string &viewState);

protected:
  CViewStateSettings();
  CViewStateSettings(const CViewStateSettings&);
  CViewStateSettings const& operator=(CViewStateSettings const&);
  virtual ~CViewStateSettings();

private:
  std::map<std::string, CViewState*> m_viewStates;
  CCriticalSection m_critical;

  void AddViewState(const std::string& strTagName, int defaultView = DEFAULT_VIEW_LIST, SORT_METHOD defaultSort = SORT_METHOD_LABEL);
};
