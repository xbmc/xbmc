/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

class CFileItem;

namespace XFILE
{
/*!
 \ingroup filesystem
 \brief Get access to a directory of a file system.

 The Factory can be used to create a directory object
 for every file system accessable. \n
 \n
 Example:

 \verbatim
 std::string strShare="iso9660://";

 IDirectory* pDir=CDirectoryFactory::Create(strShare);
 \endverbatim
 The \e pDir pointer can be used to access a directory and retrieve it's content.

 When different types of shares have to be accessed use CVirtualDirectory.
 \sa IDirectory
 */
class CDirectoryFactory
{
public:
  static IDirectory* Create(const CURL& url);
  static IDirectory* Create(const CFileItem& item);
};
}
