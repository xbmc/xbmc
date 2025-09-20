
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
#include <functional>
#include <optional>
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
 *        Lookups have log(n) complexity if Key is comparable using std::less<>,
 *        otherwise it's linear.
 */
template<typename Key, typename Value, size_t Size>
class CMap
{
public:
  template<typename Iterable>
  consteval CMap(Iterable begin, Iterable end)
  {
    std::move(begin, end, m_map.begin());

    if constexpr (requires(Key k) { std::less<>{}(k, k); })
    {
      std::sort(m_map.begin(), m_map.end(),
                [](const auto& a, const auto& b) { return std::less<>{}(a.first, b.first); });
      if (std::ranges::adjacent_find(m_map, {}, &std::pair<Key, Value>::first) != m_map.cend())
        throw std::runtime_error("Keys are not unique");
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
    if constexpr (requires(Key k) { std::less<>{}(k, k); })
    {
      const auto iter =
          std::lower_bound(m_map.cbegin(), m_map.cend(), key,
                           [](const auto& a, const auto& b) { return std::less<>{}(a.first, b); });
      if (iter != m_map.end() && !(key < iter->first))
        return iter;
      else
        return m_map.end();
    }
    else
    {
      return std::find_if(m_map.cbegin(), m_map.cend(),
                          [&key](const auto& pair) { return pair.first == key; });
    }
  }

  std::optional<Value> get(const Key& key) const
  {
    auto iter = find(key);
    if (iter != cend())
      return iter->second;
    else
      return {};
  }

  constexpr bool contains(const Key& key) const { return find(key) != cend(); }
  constexpr size_t size() const { return Size; }

  constexpr auto cbegin() const { return m_map.cbegin(); }
  constexpr auto cend() const { return m_map.cend(); }

  constexpr auto begin() const { return m_map.begin(); }
  constexpr auto end() const { return m_map.end(); }

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
consteval auto make_map(std::pair<Key, Value> (&&m)[Size]) -> CMap<Key, Value, Size>
{
  return CMap<Key, Value, Size>(std::begin(m), std::end(m));
}
