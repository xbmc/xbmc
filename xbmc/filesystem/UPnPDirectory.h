/*
 * UPnP Support for XBMC
 *  Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

class CFileItem;
class CURL;

namespace XFILE
{
class CUPnPDirectory :  public IDirectory
{
public:
    CUPnPDirectory(void) = default;
    ~CUPnPDirectory(void) override = default;

    // IDirectory methods
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool AllowAll() const override { return true; }
    bool Resolve(CFileItem& item) const override;

    // class methods
    static std::string GetFriendlyName(const CURL& url);
    static bool        GetResource(const CURL &path, CFileItem& item);
};
}
