#ifndef XBTFWRITER_H_
#define XBTFWRITER_H_

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <vector>
#include <string>
#include <stdio.h>

class CXBTF;

class CXBTFWriter
{
public:
  CXBTFWriter(CXBTF& xbtf, const std::string& outputFile);
  bool Create();
  bool Close();
  bool AppendContent(unsigned char const* data, size_t length);
  bool UpdateHeader(const std::vector<unsigned int>& dupes);

private:
  void Cleanup();

  CXBTF& m_xbtf;
  std::string m_outputFile;
  FILE* m_file;
  unsigned char *m_data;
  size_t         m_size;
};

#endif
