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

#include "Addon.h"

typedef struct cp_plugin_info_t cp_plugin_info_t;

namespace ADDON
{

class CPluginSource : public CAddon
{
public:

  enum Content { UNKNOWN, AUDIO, IMAGE, EXECUTABLE, VIDEO };

  CPluginSource(cp_plugin_info_t *props, const std::set<CPluginSource::Content>&);
  CPluginSource(const AddonProps &props) : CAddon(props) {}
  virtual ~CPluginSource() {}
  bool Provides(const Content& content) {
    return content == UNKNOWN ? false : m_providedContent.count(content); }
  static Content Translate(const CStdString &content);

private:
  std::set<Content> m_providedContent;
};

} /*namespace ADDON*/
