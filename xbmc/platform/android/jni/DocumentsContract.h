#pragma once
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

#include "JNIBase.h"

class CJNIURI;

class CJNIDocumentsContract : public CJNIBase
{
public:
  static void PopulateStaticFields();

  static std::string getTreeDocumentId (const CJNIURI& documentUri);
  static std::string getDocumentId (const CJNIURI& documentUri);
  static CJNIURI buildChildDocumentsUriUsingTree (const CJNIURI& treeUri, const std::string& parentDocumentId);
  static CJNIURI buildChildDocumentsUri (const std::string& authority, const std::string& parentDocumentId);
  static CJNIURI buildDocumentUriUsingTree  (const CJNIURI& treeUri, const std::string& parentDocumentId);

protected:
  CJNIDocumentsContract();
  ~CJNIDocumentsContract(){}
};
