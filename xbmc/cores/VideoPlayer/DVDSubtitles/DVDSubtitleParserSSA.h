#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "DVDSubtitleParser.h"
#include "DVDSubtitlesLibass.h"

#include <memory>

class CDVDSubtitleParserSSA : public CDVDSubtitleParserText
{
public:
  CDVDSubtitleParserSSA(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& strFile);
  ~CDVDSubtitleParserSSA() override;

  bool Open(CDVDStreamInfo &hints) override;
  void Dispose() override;

private:
  CDVDSubtitlesLibass* m_libass;
};
