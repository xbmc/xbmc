
/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <array>
#include <stdexcept>

/*!
 * \brief This class is designed to implement a constexpr version of std::map.
 *        The standard library std::map doesn't allow constexpr (and it
 *        doesn't look like it will be implemented in the future). This class
 *        utilizes std::array and std::pair as they allow constexpr.
 *
 *        When using this class you should use the helper make_map instead of
 *        constructing this class directly. For example:
 *        constexpr auto myMap = make_map<int, std::string_view>({{1, "one"}});
 *
 *        This class is useful for mapping enum values to strings that can be
 *        compile time checked. This also helps with heap usage.
 *
 *        Lookups have linear complexity, so should not be used for "big" maps.
 */
template<typename Key, typename Value, size_t Size>
class CMap
{
public:
  template<typename Iterable>
  constexpr CMap(Iterable begin, Iterable end)
  {
    size_t index = 0;
    while (begin != end)
    {
      // c++17 doesn't have constexpr assignment operator for std::pair
      auto& first = m_map[index].first;
      auto& second = m_map[index].second;
      ++index;

      first = std::move(begin->first);
      second = std::move(begin->second);
      ++begin;

      //! @todo: c++20 can use constexpr assignment operator instead
      // auto& p = data[index];
      // ++index;

      // p = std::move(*begin);
      // ++begin;
      //
    }
  }

  ~CMap() = default;

  constexpr const Value& at(const Key& key) const
  {
    const auto it = find(key);
    if (it != m_map.cend())
    {
      return it->second;
    }
    else
    {
      throw std::range_error("Not Found");
    }
  }

  constexpr auto find(const Key& key) const
  {
    return std::find_if(m_map.cbegin(), m_map.cend(),
                        [&key](const auto& pair) { return pair.first == key; });
  }

  constexpr size_t size() const { return Size; }

  constexpr auto cbegin() const { return m_map.cbegin(); }
  constexpr auto cend() const { return m_map.cend(); }

private:
  CMap() = delete;

  std::array<std::pair<Key, Value>, Size> m_map;
};

/*!
 * \brief Use this helper when wanting to use CMap. This is needed to allow
 *        deducing the size of the map from the initializer list without
 *        needing to explicitly give the size of the map (similar to std::map).
 *
 */
template<typename Key, typename Value, std::size_t Size>
constexpr auto make_map(std::pair<Key, Value>(&&m)[Size]) -> CMap<Key, Value, Size>
{
  return CMap<Key, Value, Size>(std::begin(m), std::end(m));
}
