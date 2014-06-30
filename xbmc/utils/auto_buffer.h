#pragma once
/*
*      Copyright (C) 2013-2014 Team XBMC
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

#include <stddef.h> // for size_t

namespace XUTILS
{

  class auto_buffer
  {
  public:
    auto_buffer(void) : p(NULL), s(0)
    {}
    explicit auto_buffer(size_t size);
    ~auto_buffer();

    auto_buffer& allocate(size_t size);
    auto_buffer& resize(size_t newSize);
    auto_buffer& clear(void);

    inline char* get(void) const { return static_cast<char*>(p); }
    inline size_t size(void) const { return s; }
    inline size_t length(void) const { return s; }

    auto_buffer& attach(void* pointer, size_t size);
    void* detach(void);

  private:
    auto_buffer(const auto_buffer& other); // disallow copy constructor
    auto_buffer& operator=(const auto_buffer& other); // disallow assignment

    void* p;
    size_t s;
  };
}
