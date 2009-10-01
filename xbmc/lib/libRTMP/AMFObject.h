#ifndef __AMF_OBJECT__H__
#define __AMF_OBJECT__H__
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This file is part of libRTMP.
 *
 *  libRTMP is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  libRTMP is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with libRTMP; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string>
#include <vector>

#ifdef _WIN32
#undef GetObject // WIN32INCLUDES defined in WinGDI.h which appears to be included from _somewhere_
#endif

namespace RTMP_LIB
{
  typedef enum {AMF_INVALID, AMF_NUMBER, AMF_BOOLEAN, AMF_STRING, AMF_OBJECT, AMF_NULL } AMFDataType;

  class AMFObjectProperty;
  class AMFObject
  {
    public:
      AMFObject();
      virtual ~AMFObject();

      int Encode(char *pBuffer, int nSize) const;
      int Decode(const char *pBuffer, int nSize, bool bDecodeName=false);

      void AddProperty(const AMFObjectProperty &prop);

      int GetPropertyCount() const;
      const AMFObjectProperty &GetProperty(const std::string &strName) const;
      const AMFObjectProperty &GetProperty(size_t nIndex) const;

      void Dump() const;
      void Reset();
    protected:
      static AMFObjectProperty m_invalidProp; // returned when no prop matches
      std::vector<AMFObjectProperty> m_properties;
  };

  class AMFObjectProperty
  {
    public:
      AMFObjectProperty();
      AMFObjectProperty(const std::string &strName, double dValue);
      AMFObjectProperty(const std::string &strName, bool   bValue);
      AMFObjectProperty(const std::string &strName, const std::string &strValue);
      AMFObjectProperty(const std::string &strName, const AMFObject &objValue);
     
      virtual ~AMFObjectProperty();

      const std::string &GetPropName() const;

      AMFDataType GetType() const;

      bool IsValid() const;

      double GetNumber() const;
      bool   GetBoolean() const;
      const std::string &GetString() const;
      const AMFObject &GetObject() const;

      int Encode(char *pBuffer, int nSize) const;
      int Decode(const char *pBuffer, int nSize, bool bDecodeName);

      void Reset();
      void Dump() const;
    protected:
      int EncodeName(char *pBuffer) const;

      std::string m_strName;

      AMFDataType m_type;
      double      m_dNumVal;
      AMFObject   m_objVal;
      std::string m_strVal;
  };

};

#endif
