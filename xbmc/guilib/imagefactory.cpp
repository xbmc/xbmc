/*
 *      Copyright (C) 2012-present Team Kodi
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

#include "imagefactory.h"
#include "guilib/FFmpegImage.h"
#include "addons/ImageDecoder.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

#include <algorithm>

using namespace ADDON;

IImage* ImageFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IImage* ImageFactory::CreateLoader(const CURL& url)
{
  if(!url.GetFileType().empty())
    return CreateLoaderFromMimeType("image/"+url.GetFileType());

  return CreateLoaderFromMimeType(CMime::GetMimeType(url));
}

IImage* ImageFactory::CreateLoaderFromMimeType(const std::string& strMimeType)
{
  BinaryAddonBaseList addonInfos;

  CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, true, ADDON_IMAGEDECODER);
  for (auto addonInfo : addonInfos)
  {
    std::vector<std::string> mime = StringUtils::Split(addonInfo->Type(ADDON_IMAGEDECODER)->GetValue("@mimetype").asString(), "|");
    if (std::find(mime.begin(), mime.end(), strMimeType) != mime.end())
    {
      CImageDecoder* result = new CImageDecoder(addonInfo);
      result->Create(strMimeType);
      return result;
    }
  }

  return new CFFmpegImage(strMimeType);
}
