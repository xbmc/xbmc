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

#include "DVDInputStream.h"
#include "boost/shared_ptr.hpp"

class CDVDInputStreamStack : public CDVDInputStream
{
public:
  CDVDInputStreamStack();
  virtual ~CDVDInputStreamStack();

  virtual bool    Open(const char* path, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool    IsEOF();
  virtual int64_t GetLength();

protected:

  typedef boost::shared_ptr<XFILE::CFile> TFile;

  struct TSeg
  {
    TFile   file;
    int64_t length;
  };

  typedef std::vector<TSeg> TSegVec;

  TSegVec m_files;  ///< collection of open ptr's to all files in stack
  TFile   m_file;   ///< currently active file
  bool    m_eof;
  int64_t m_pos;
  int64_t m_length;
};
