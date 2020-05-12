/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>

template<typename Key, typename Value, class Map = std::map<Key, Value>>
class CDefaultedMap
{
public:
  using key_type = Key;
  using value_type = Value;

  CDefaultedMap()
    : CDefaultedMap("")
  {
  }

  explicit CDefaultedMap(Key initialDefault)
    : m_initialDefault(initialDefault),
      m_default(initialDefault)
  {
  }

  CDefaultedMap(const CDefaultedMap& other) = default;
  CDefaultedMap(CDefaultedMap&& other) = default;

  CDefaultedMap& operator=(const CDefaultedMap& other) = default;
  CDefaultedMap& operator=(CDefaultedMap&& other) = default;

  CDefaultedMap& operator=(Map&& map)
  {
    Set(std::move(map));
    return *this;
  }

  const Map& Get() const { return m_map; }

  const Key& GetDefault() const { return m_default; }

  bool HasDefaultValue() const { return m_map.find(m_default) != m_map.end(); }

  Value GetDefaultValue() const
  {
    if (m_map.empty())
      return {};

    return m_map[m_default];
  }

  bool HasValue(Key key) const { return m_map.find(key) != m_map.end(); }

  Value GetValue(Key key = {}) const
  {
    Value value;
    if (TryGetValue(value, key))
      return value;

    return {};
  }

  bool TryGetValue(Value& value, Key key = {}) const
  {
    if (key == Key())
      key = m_default;

    const auto val = m_map.find(key);
    if (val == m_map.end())
      return false;

    value = val->second;
    return true;
  }

  bool Empty() const { return m_map.empty(); }

  typename Map::size_type Size() const { return m_map.size(); }

  void Clear()
  {
    m_map.clear();
    m_default = m_initialDefault;
  }

  void Set(Map&& map)
  {
    // update the map and try to keep the default
    Set(std::move(map), m_default);
  }

  void Set(Map&& map, Key defaultKey)
  {
    // make sure there is a valid default
    if (map.empty())
      defaultKey = m_initialDefault;
    else if (map.find(defaultKey) == map.end())
    {
      // keep the current default if it is available
      if (map.find(m_default) != map.end())
        defaultKey = m_default;
      else if (m_initialDefault != m_default && map.find(m_initialDefault) != map.end())
        defaultKey = m_initialDefault;
      else
        defaultKey = map.begin()->first;
    }

    m_map = std::move(map);
    m_default = std::move(defaultKey);
  }

  bool SetDefault(Key defaultKey)
  {
    if (m_map.find(defaultKey) != m_map.end())
    {
      m_default = std::move(defaultKey);
      return true;
    }

    return false;
  }

  bool SetValue(Value value, Key key = {}, bool isDefault = false)
  {
    if (key == Key())
      key = m_default;

    if (isDefault || m_map.empty())
      m_default = key;

    m_map[std::move(key)] = std::move(value);

    return true;
  }

  bool RemoveValue(Key key)
  {
    if (m_map.erase(key) == 0)
      return false;

    // update the default if it was just removed
    if (key == m_default)
    {
      if (m_map.empty())
        m_default = m_initialDefault;
      else
        m_default = m_map.begin()->first;
    }

    return true;
  }

private:
  Key m_initialDefault;

  Map m_map;
  Key m_default;
};
