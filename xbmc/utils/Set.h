/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <stdexcept>

/*!
 * \brief A `constexpr` set, like `std::set` but at compile time
 *
 * This class is designed to implement a `constexpr` version of std::set.
 *
 * When using this class, "class template argument deduction" will figure out
 * the type and size for you:
 *
 * ```
 * constexpr CSet magicNumber{5738, 573942, 5832}; // --> CSet<int, 3>
 * ```
 *
 * If you want to force a type, but not the size, use the
 * `make_set` helper function like:
 *
 * ```
 * constexpr auto mySet = make_set<std::string_view>({"one", "two", "three"});
 * ```
 *
 * The elements in CSet need to be totally ordered, this allows for O(log n)
 * complexity when looking up values.
 */
template<std::totally_ordered Key, size_t Size>
class CSet
{
public:
  consteval CSet(std::initializer_list<Key> keys) : CSet(keys.begin(), keys.end()) {}

  template<std::forward_iterator Iter>
  consteval CSet(Iter begin, Iter end)
  {
    std::ranges::subrange keys(begin, end);
    std::ranges::move(keys, m_keys.begin());
    std::ranges::sort(m_keys);
    if (std::ranges::adjacent_find(m_keys) != m_keys.end())
      throw std::runtime_error("CSet does not support duplicate elements");
  }

  ~CSet() = default;

  constexpr bool contains(const Key& key) const { return std::ranges::binary_search(m_keys, key); }
  constexpr size_t size() const { return Size; }
  constexpr bool empty() const { return Size == 0; }

  constexpr auto cbegin() const { return m_keys.cbegin(); }
  constexpr auto cend() const { return m_keys.cend(); }

  constexpr auto begin() const { return m_keys.cbegin(); }
  constexpr auto end() const { return m_keys.cend(); }

private:
  CSet() = delete;

  std::array<Key, Size> m_keys;
};

// class template argument deduction guide

template<class T, class... U>
  requires(... && std::convertible_to<U, T>)
CSet(T, U...) -> CSet<T, 1 + sizeof...(U)>;

/*!
 * \brief Helper if the Key type of the CSet should be different than the provided values
 */
template<typename Key, std::size_t Size>
consteval auto make_set(Key (&&k)[Size]) -> CSet<Key, Size>
{
  return CSet<Key, Size>(std::begin(k), std::end(k));
}
