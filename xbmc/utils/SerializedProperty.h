/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <string>

class ISerializedProperty
{
public:
  virtual ~ISerializedProperty() = default;

  virtual void FromString(const std::string& json) = 0;

  virtual std::string ToString() const = 0;
};

template<typename T>
class CSerializedProperty : public ISerializedProperty
{
public:
  using Serializer = std::function<std::string(const T&)>;
  using Deserializer = std::function<T(const std::string&)>;

  template<typename... Args>
  CSerializedProperty(Serializer&& serializer, Deserializer&& deserializer, Args&&... args)
    : m_serializer(std::move(serializer)), m_deserializer(std::move(deserializer)), m_value(std::forward<Args>(args)...)
  {
  }

  CSerializedProperty(const CSerializedProperty& other)
    : m_serializer(other.m_serializer), m_deserializer(other.m_deserializer), m_value(other.m_value) {}

  CSerializedProperty(CSerializedProperty&& other)
    : m_serializer(std::move(other.m_serializer)), m_deserializer(std::move(other.m_deserializer)), m_value(std::move(other.m_value)) {}

  CSerializedProperty& operator=(const CSerializedProperty& other)
  {
    m_serializer = other.m_serializer;
    m_deserializer = other.m_deserializer;
    m_value = other.m_value;
    return *this;
  }

  CSerializedProperty& operator=(CSerializedProperty&& other)
  {
    m_serializer = std::move(other.m_serializer);
    m_deserializer = std::move(other.m_deserializer);
    m_value = std::move(other.m_value);
    return *this;
  }

  CSerializedProperty& operator=(T&& value)
  {
    m_value = std::move(value);
    return *this;
  }

  T* operator->() { return &value(); }
  const T* operator->() const { return &value(); }

  T& operator*() { return value(); }
  const T& operator*() const { return value(); }

  operator T&() { return value(); }
  operator const T&() const { return value(); }

  virtual T& value() { return m_value; }
  virtual const T& value() const { return m_value; }

  // implementations of ISerializedProperty
  void FromString(const std::string& json) override { m_value = m_deserializer(json); }

  std::string ToString() const override { return m_serializer(m_value); }

protected:
  Serializer m_serializer;
  Deserializer m_deserializer;

  mutable T m_value;
};
