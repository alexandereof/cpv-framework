#pragma once
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <utility>

namespace cpv {
	/**
	 * A cache type that keeps up to given number of least recently used values.
	 *
	 * Notice:
	 * It's not thread safe, don't use it across threads without mutex.
	 */
	template <
		class Key,
		class Value,
		class List = std::list<std::pair<Key, Value>>,
		// use ordered map and std::less<> by default for heterogeneous lookup
		class Map = std::map<Key, typename List::iterator, std::less<>>>
	class LRUCache {
	public:
		/** Associate value with key, remove finally not used value if size is over */
		template <class TKey, class TValue>
		void set(TKey&& keyForMap, TKey&& keyForList, TValue&& value) {
			assert(keyForMap == keyForList);
			auto it = map_.find(keyForMap);
			if (it != map_.end()) {
				list_.erase(it->second);
				map_.erase(it);
			}
			list_.emplace_front(
				std::forward<TKey>(keyForList), std::forward<TValue>(value));
			map_.emplace(std::forward<TKey>(keyForMap), list_.begin());
			if (map_.size() > maxSize_) {
				map_.erase(list_.back().first);
				list_.pop_back();
			}
		}

		/** Associate value with key, remove finally not used value if size is over */
		template <class TKey, class TValue>
		void set(const TKey& key, TValue&& value) {
			set(TKey(key), TKey(key), std::forward<TValue>(value));
		}

		/** Associate value with key, remove finally not used value if size is over */
		void set(Key&& keyForMap, Key&& keyForList, Value&& value) {
			assert(keyForMap == keyForList);
			set<Key, Value>(std::move(keyForMap), std::move(keyForList), std::move(value));
		}

		/** Associate value with key, remove finally not used value if size is over */
		void set(const Key& key, Value&& value) {
			set<Key, Value>(Key(key), Key(key), std::move(value));
		}

		/** Get pointer of value associated with key or return nullptr */
		template <class TKey>
		Value* get(TKey&& key) & {
			auto it = map_.find(std::forward<TKey>(key));
			if (it == map_.end()) {
				return nullptr;
			} else {
				list_.splice(list_.begin(), list_, it->second);
				return &it->second->second;
			}
		}

		/** Get pointer of value associated with key or return nullptr */
		Value* get(const Key& key) & {
			return get<const Key&>(key);
		}

		/** Erase value associated with key, return whether key was exists */
		template <class TKey>
		bool erase(TKey&& key) {
			auto it = map_.find(std::forward<TKey>(key));
			if (it == map_.end()) {
				return false;
			} else {
				list_.erase(it->second);
				map_.erase(it);
				return true;
			}
		}

		/** Erase value associated with key, return whether key was exists */
		bool erase(const Key& key) {
			return erase<const Key&>(key);
		}

		/** Erase all values in cache */
		void clear() {
			list_.clear();
			map_.clear();
		}

		/** Get the number of values in cache */
		std::size_t size() const {
			return map_.size();
		}

		/** Get the maximum number of values allow to keep in cache */
		std::size_t maxSize() const {
			return maxSize_;
		}

		/** Return whether the cache is empty */
		bool empty() const {
			return map_.empty();
		}

		/** Construct with max number of values allow to keep in cache */
		LRUCache(std::size_t maxSize) :
			list_(),
			map_(),
			maxSize_(maxSize) { }

	private:
		List list_;
		Map map_;
		std::size_t maxSize_;
	};
}

