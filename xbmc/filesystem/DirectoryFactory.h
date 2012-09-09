#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "IDirectory.h"

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
 CStdString strShare="iso9660://";

 IDirectory* pDir=CDirectoryFactory::Create(strShare);
 \endverbatim
 The \e pDir pointer can be used to access a directory and retrieve it's content.

 When different types of shares have to be accessed use CVirtualDirectory.
 \sa IDirectory
 */
class CDirectoryFactory
{
public:
  static IDirectory* Create(const CStdString& strPath);
};
}
