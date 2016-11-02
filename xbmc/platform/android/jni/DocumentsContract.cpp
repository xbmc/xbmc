/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "DocumentsContract.h"
#include "URI.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

void CJNIDocumentsContract::PopulateStaticFields()
{
}

std::string CJNIDocumentsContract::getTreeDocumentId(const CJNIURI &documentUri)
{
  return jcast<std::string>(call_static_method<jhstring>("android/provider/DocumentsContract",
                                                         "getTreeDocumentId", "(Landroid/net/Uri;)Ljava/lang/String;", documentUri.get_raw()));
}

std::string CJNIDocumentsContract::getDocumentId(const CJNIURI &documentUri)
{
  return jcast<std::string>(call_static_method<jhstring>("android/provider/DocumentsContract",
                                                         "getDocumentId", "(Landroid/net/Uri;)Ljava/lang/String;", documentUri.get_raw()));
}

CJNIURI CJNIDocumentsContract::buildChildDocumentsUri(const std::string &authority, const std::string &parentDocumentId)
{
  return (CJNIURI)call_static_method<jhobject>("android/provider/DocumentsContract",
    "buildChildDocumentsUri", "(Ljava/lang/String;Ljava/lang/String;)Landroid/net/Uri;", jcast<jhstring>(authority), jcast<jhstring>(parentDocumentId));
}

CJNIURI CJNIDocumentsContract::buildChildDocumentsUriUsingTree(const CJNIURI &treeUri, const std::string& parentDocumentId)
{
  return (CJNIURI)call_static_method<jhobject>("android/provider/DocumentsContract",
    "buildChildDocumentsUriUsingTree", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;", treeUri.get_raw(), jcast<jhstring>(parentDocumentId));
}

CJNIURI CJNIDocumentsContract::buildDocumentUriUsingTree(const CJNIURI &treeUri, const std::string& parentDocumentId)
{
  return (CJNIURI)call_static_method<jhobject>("android/provider/DocumentsContract",
    "buildDocumentUriUsingTree", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;", treeUri.get_raw(), jcast<jhstring>(parentDocumentId));
}
