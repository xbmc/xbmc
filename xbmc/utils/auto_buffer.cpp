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

#include "auto_buffer.h"
#include <new> // for std::bad_alloc
#include <stdlib.h> // for malloc(), realloc() and free()

using namespace XUTILS;

auto_buffer::auto_buffer(size_t size) : p(0), s(0)
{
  if (!size)
    return;

  p = malloc(size); // "malloc()" instead of "new" allow to use "realloc()"
  if (!p)
    throw std::bad_alloc();
  s = size;
}

auto_buffer::~auto_buffer()
{
  free(p);
}

auto_buffer& auto_buffer::allocate(size_t size)
{
  clear();
  if (size)
  {
    p = malloc(size);
    if (!p)
      throw std::bad_alloc();
    s = size;
  }
  return *this;
}

auto_buffer& auto_buffer::resize(size_t newSize)
{
  if (!newSize)
    return clear();

  void* newPtr = realloc(p, newSize);
  if (!newPtr)
    throw std::bad_alloc();
  p = newPtr;
  s = newSize;
  return *this;
}

auto_buffer& auto_buffer::clear(void)
{
  free(p);
  p = 0;
  s = 0;
  return *this;
}

auto_buffer& auto_buffer::attach(void* pointer, size_t size)
{
  clear();
  if ((pointer && size) || (!pointer && !size))
  {
    p = pointer;
    s = size;
  }
  return *this;
}

void* auto_buffer::detach(void)
{
  void* returnPtr = p;
  p = 0;
  s = 0;
  return returnPtr;
}

