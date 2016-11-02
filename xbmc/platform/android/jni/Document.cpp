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

#include "Document.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNIDocument::COLUMN_DISPLAY_NAME;
std::string CJNIDocument::COLUMN_MIME_TYPE;
std::string CJNIDocument::COLUMN_DOCUMENT_ID;
std::string CJNIDocument::COLUMN_SIZE;
std::string CJNIDocument::COLUMN_FLAGS;
std::string CJNIDocument::MIME_TYPE_DIR;

void CJNIDocument::PopulateStaticFields()
{
  if (CJNIBase::GetSDKVersion() >= 19)
  {
    jhclass c = find_class("android/provider/DocumentsContract$Document");
    CJNIDocument::COLUMN_DISPLAY_NAME          = jcast<std::string>(get_static_field<jhstring>(c,"COLUMN_DISPLAY_NAME"));
    CJNIDocument::COLUMN_MIME_TYPE             = jcast<std::string>(get_static_field<jhstring>(c,"COLUMN_MIME_TYPE"));
    CJNIDocument::COLUMN_DOCUMENT_ID           = jcast<std::string>(get_static_field<jhstring>(c,"COLUMN_DOCUMENT_ID"));
    CJNIDocument::COLUMN_SIZE                  = jcast<std::string>(get_static_field<jhstring>(c,"COLUMN_SIZE"));
    CJNIDocument::COLUMN_FLAGS                 = jcast<std::string>(get_static_field<jhstring>(c,"COLUMN_FLAGS"));
    CJNIDocument::MIME_TYPE_DIR                = jcast<std::string>(get_static_field<jhstring>(c,"MIME_TYPE_DIR"));
  }
}
