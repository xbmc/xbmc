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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

 IDirectory* pDir=CFactoryDirectory::Create(strShare);
 \endverbatim
 The \e pDir pointer can be used to access a directory and retrieve it's content.

 When different types of shares have to be accessed use CVirtualDirectory.
 \sa IDirectory
 */
class CFactoryDirectory
{
public:
  static IDirectory* Create(const CStdString& strPath);
};
}
