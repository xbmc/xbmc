#pragma once

#include <unordered_map>
#include <deque>
#include <limits>

template<typename K, typename V, size_t MaxSize = 512>
class AgedMap {
public:
  void insert(K key, V value);
  auto find(const K& key) const { return map.find(key); }
  auto findOrLatest(const K& key) const;
  auto end() const { return map.end(); }

private:
  std::unordered_map<K, V> map;
  std::deque<K> ages;
  K mostRecentKey;
  bool hasEntries = false;
};

template<typename K, typename V, size_t MaxSize>
void AgedMap<K, V, MaxSize>::insert(K key, V value) {
  bool isNewKey = (map.find(key) == map.end());

  map[key] = value;
  mostRecentKey = key;
  hasEntries = true;

  if (isNewKey) {
    ages.push_back(key);

    while (map.size() > MaxSize) {
      map.erase(ages.front());
      ages.pop_front();
    }
  }
}

template<typename K, typename V, size_t MaxSize>
auto AgedMap<K, V, MaxSize>::findOrLatest(const K& key) const {
  auto exact = map.find(key);

  if (exact != map.end()) return exact;
  if (!hasEntries) return map.end();

  auto closest = map.end();
  typename std::unordered_map<K, V>::size_type minDistance =
      std::numeric_limits<typename std::unordered_map<K, V>::size_type>::max();

  for (auto it = map.begin(); it != map.end(); ++it) {
    typename std::unordered_map<K, V>::size_type distance;
    if (it->first > key)
      distance = it->first - key;
    else
      distance = key - it->first;

    if (distance < minDistance) {
      minDistance = distance;
      closest = it;
    }
  }

  return closest;
}