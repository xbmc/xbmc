/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SerializedProperty.h"

template<typename T>
class CLazySerializedProperty : public CSerializedProperty<T>
{
public:
  template<typename... Args>
  CLazySerializedProperty(Serializer&& serializer, Deserializer&& deserializer, Args&&... args)
    : CSerializedProperty(std::move(serializer), std::move(deserializer), std::forward<Args>(args)...)
  {
  }

  CLazySerializedProperty(const CLazySerializedProperty& other) : CSerializedProperty(other), m_data(other.m_data), m_parsed(other.m_parsed) {}

  CLazySerializedProperty(CLazySerializedProperty&& other) : CSerializedProperty(std::move(other)), m_data(std::move(other.m_data)), m_parsed(std::move(other.m_parsed)) {}

  CLazySerializedProperty& operator=(const CLazySerializedProperty& other)
  {
    CSerializedProperty::operator=(other);
    m_data = other.m_data;
    m_parsed = other.m_parsed;
    return *this;
  }
  CLazySerializedProperty& operator=(CLazySerializedProperty&& other)
  {
    CSerializedProperty::operator=(std::move(other));
    m_data = std::move(other.m_data);
    m_parsed = std::move(other.m_parsed);
    return *this;
  }

  CLazySerializedProperty& operator=(T&& value)
  {
    m_value = std::move(value);
    m_parsed = true;
    return *this;
  }

  // specializations of of CSerializedProperty
  virtual T& value()
  {
    deserialize();
    return m_value;
  }
  virtual const T& value() const
  {
    deserialize();
    return m_value;
  }

  void FromString(const std::string& json) override
  {
    m_data = json;
    m_parsed = false;
  }

  std::string ToString() const override
  {
    if (!m_parsed)
      return m_data;

    return m_serializer(m_value);
  }

protected:
  void deserialize() const
  {
    if (m_parsed)
      return;

    m_value = m_deserializer(m_data);
    m_parsed = true;
  }

  std::string m_data;
  mutable bool m_parsed = false;
};
