/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

namespace ssf
{
  // simple array class for simple types without constructors, 
  // and it doesn't free its reserves on SetCount(0)

  template<class T> 
  class Array
  {
    T* m_pData;
    size_t m_nSize;
    size_t m_nMaxSize;
    size_t m_nGrowBy;

  public:
    Array() {m_pData = NULL; m_nSize = m_nMaxSize = 0; m_nGrowBy = 4096;}
    virtual ~Array() {if(m_pData) _aligned_free(m_pData);}

    void SetCount(size_t nSize, size_t nGrowBy = 0)
    {
      if(nGrowBy > 0)
      {
        m_nGrowBy = nGrowBy;
      }

      if(nSize > m_nMaxSize)
      {
        m_nMaxSize = nSize + max(m_nGrowBy, m_nSize);
        size_t nBytes = m_nMaxSize * sizeof(T);
        m_pData = m_pData ? (T*)_aligned_realloc(m_pData, nBytes, 16) : (T*)_aligned_malloc(nBytes, 16);
      }

      m_nSize = nSize;
    }

    size_t size() const {return m_nSize;}

    void clear() {m_nSize = 0;}
    bool empty() const {return m_nSize == 0;}

    T* GetData() {return m_pData;}

    void Add(const T& t)
    {
      size_t nPos = m_nSize;
      SetCount(m_nSize+1);
      m_pData[nPos] = t;
    }

    void Append(const Array& a, size_t nGrowBy = 0)
    {
      Append(a.m_pData, a.m_nSize, nGrowBy);
    }

    void Append(const T* ptr, size_t nSize, size_t nGrowBy = 0)
    {
      if(!nSize) return;
      size_t nOldSize = m_nSize;
      SetCount(nOldSize + nSize);
      memcpy(m_pData + nOldSize, ptr, nSize * sizeof(T));
    }

    const T& operator [] (size_t i) const {return m_pData[i];}
    T& operator [] (size_t i) {return m_pData[i];}

    void Copy(const Array& v)
    {
      SetCount(v.size());
      memcpy(m_pData, v.m_pData, m_nSize * sizeof(T));
    }

    void Move(Array& v)
    {
      Swap(v);
      v.SetCount(0);
    }

    void Swap(Array& v)
    {
      T* pData = m_pData; m_pData = v.m_pData; v.m_pData = pData;
      size_t nSize = m_nSize; m_nSize = v.m_nSize; v.m_nSize = nSize;
      size_t nMaxSize = m_nMaxSize; m_nMaxSize = v.m_nMaxSize; v.m_nMaxSize = nMaxSize;
      size_t nGrowBy = m_nGrowBy; m_nGrowBy = v.m_nGrowBy; v.m_nGrowBy = nGrowBy;
    }
  };
}