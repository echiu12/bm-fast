#ifndef __LZ77_APPROX__HASH_TABLE_HPP_INCLUDED
#define __LZ77_APPROX__HASH_TABLE_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include "space_efficient_vector.hpp"
#include "utils.hpp"

#ifndef LZ77_APPROX_HASHTABLE_INDICES_PER_NODE
#define LZ77_APPROX_HASHTABLE_INDICES_PER_NODE 8
#endif

#ifndef LZ77_APPROX_HASHTABLE_ENABLE_JUMPING
#define LZ77_APPROX_HASHTABLE_ENABLE_JUMPING 1
#endif

namespace lz77_approx {

////////////////////////////////////
// FingerprintHashTable Interface //
////////////////////////////////////

// fingerprint hash table iterator functions.
template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_dereferemce_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_dereferemce_operator<T, std::enable_if_t<std::is_same_v<typename T::reference, decltype(std::declval<const T>().operator*())>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_arrow_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_arrow_operator<T, std::enable_if_t<std::is_same_v<typename T::pointer, decltype(std::declval<const T>().operator->())>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_preincrement_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_preincrement_operator<T, std::enable_if_t<std::is_same_v<T&, decltype(std::declval<T>().operator++())>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_postincrement_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_postincrement_operator<T, std::enable_if_t<std::is_same_v<T, decltype(std::declval<T>().operator++(std::declval<int>()))>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_equality_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_equality_operator<T, std::enable_if_t<std::is_same_v<bool, decltype(std::declval<const T>().operator==(std::declval<const T&>()))>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_iterator_inequality_operator : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_iterator_inequality_operator<T, std::enable_if_t<std::is_same_v<bool, decltype(std::declval<const T>().operator!=(std::declval<const T&>()))>, void>> : std::true_type {};

// is_fingerprint_hash_table_iterator.
template<typename T, typename = void> struct is_fingerprint_hash_table_iterator : std::false_type {};
template<typename T> struct is_fingerprint_hash_table_iterator<T, std::enable_if_t<std::conjunction_v<
  has_fingerprint_hash_table_iterator_dereferemce_operator<T>,
  has_fingerprint_hash_table_iterator_arrow_operator<T>,
  has_fingerprint_hash_table_iterator_preincrement_operator<T>,
  has_fingerprint_hash_table_iterator_postincrement_operator<T>,
  has_fingerprint_hash_table_iterator_equality_operator<T>,
  has_fingerprint_hash_table_iterator_inequality_operator<T>
>, void>> : std::true_type {}; 

// fingerprint hash table functions.
template<typename T, typename = void> struct has_fingerprint_hash_table_find : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_find<T, std::enable_if_t<std::is_same_v<typename T::iterator_type, decltype(std::declval<T>().find(std::declval<typename T::fingerprint_type>()))>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_end : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_end<T, std::enable_if_t<std::is_same_v<typename T::iterator_type, decltype(std::declval<T>().end())>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_insert : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_insert<T, std::enable_if_t<std::is_same_v<void, decltype(std::declval<T>().insert(std::declval<typename T::fingerprint_type>(), std::declval<typename T::value_type>()))>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_insert_v2 : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_insert_v2<T, std::enable_if_t<std::is_same_v<void, decltype(std::declval<T>().insert(std::declval<typename T::fingerprint_type>(), std::declval<typename T::value_type>(), std::declval<typename T::iterator_type&>()))>, void>> : std::true_type {};

template<typename T, typename = void> struct has_fingerprint_hash_table_set_match_results : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_set_match_results<T, std::void_t<
    std::enable_if_t<std::is_same_v<
        void,
        decltype(std::declval<T>().set_match_results(
            std::declval<typename T::iterator_type&>(),
            std::declval<typename T::size_type>(),
            std::declval<bool>()
        ))
    >, void>
>> : std::true_type {}; 

template<typename T, typename = void> struct has_fingerprint_hash_table_get_prefix_length : std::false_type {};
template<typename T> struct has_fingerprint_hash_table_get_prefix_length<T, std::enable_if_t<std::is_same_v<
    typename T::size_type,
    decltype(std::declval<const T>().get_prefix_length(
        std::declval<typename T::iterator_type&>()
    ))
>>> : std::true_type {}; 

// is_fingerprint_hash_table.
template<typename T, typename = void> struct is_fingerprint_hash_table : std::false_type {};
template<typename T> struct is_fingerprint_hash_table<T, std::enable_if_t<std::disjunction_v<
  std::conjunction<
    has_fingerprint_hash_table_find<T>,
    has_fingerprint_hash_table_end<T>,
    has_fingerprint_hash_table_insert<T>,
    is_fingerprint_hash_table_iterator<typename T::iterator_type>
  >,
  std::conjunction<
    has_fingerprint_hash_table_find<T>,
    has_fingerprint_hash_table_end<T>,
    has_fingerprint_hash_table_insert_v2<T>,
    has_fingerprint_hash_table_set_match_results<T>,
    has_fingerprint_hash_table_get_prefix_length<T>,
    is_fingerprint_hash_table_iterator<typename T::iterator_type>
  >
>, void>> : std::true_type {}; 

///////////////////////////
// Default hash function //
///////////////////////////

struct BasicHashFunction {
  inline static uint64_t hash(uint64_t value) { return value; }
};

/////////////////////
// Hash table item //
/////////////////////

template<typename S, typename T, typename U = uint64_t>
struct HashTableItem {
  S m_key;
  T m_value;
  U m_next;

  HashTableItem(S key, T value, U next) : m_key(key), m_value(value), m_next(next) {}
};

////////////////
// Hash table //
////////////////

template <
  typename t_key = uint64_t,
  typename t_value = uint64_t,
  typename t_allocator = std::allocator<uint8_t>,
  typename t_item = HashTableItem<t_key, t_value>,
  typename t_bucket_list = SpaceEfficientVector<uint64_t, typename std::allocator_traits<t_allocator>::template rebind_alloc<uint64_t>>,
  typename t_item_list = SpaceEfficientVector<t_item, typename std::allocator_traits<t_allocator>::template rebind_alloc<t_item>>,
  typename t_hashing = BasicHashFunction
>
class HashTable {
public:
  using size_type = uint64_t;
  using key_type = t_key;
  using value_type = t_value;
  using allocator_type = t_allocator;
  using item_type = t_item;
  using bucket_list_type = t_bucket_list;
  using item_list_type = t_item_list;
  using hashing_type = t_hashing;
private:
  bucket_list_type m_buckets;
  item_list_type m_items;
public:
  HashTable() {
    m_buckets.push_back(std::numeric_limits<size_type>::max());
  }
  void enlarge() {
    m_buckets.resize(m_buckets.size() * 2);
    std::fill(m_buckets.begin(), m_buckets.end(), std::numeric_limits<size_type>::max());
    size_type num_buckets = m_buckets.size();
    // Rehash
    for (size_type i = 0; i < m_items.size(); ++i) {
      item_type item = m_items[i];
      size_type bucket_index = t_hashing::hash(item.m_key) & (num_buckets - 1);
      m_items[i].m_next = m_buckets[bucket_index];
      m_buckets[bucket_index] = i;
    }
  }
  // Assumes that key does not already exist (use find to check beforehand)
  value_type* insert(const key_type& key, const value_type& value) {
    if (m_items.size() == m_buckets.size()) {
      enlarge();
    }
    size_type num_buckets = m_buckets.size();
    size_type bucket_index = t_hashing::hash(key) & (num_buckets - 1);
    item_type item(key, value, m_buckets[bucket_index]);
    m_buckets[bucket_index] = m_items.size();
    m_items.push_back(item);
    return &(m_items.back().m_value);
  }
  value_type* find(const key_type& key) {
    size_type num_buckets = m_buckets.size();
    size_type bucket_index = t_hashing::hash(key) & (num_buckets - 1);
    size_type item_index = m_buckets[bucket_index];
    while (item_index != std::numeric_limits<size_type>::max()) {
      if (m_items[item_index].m_key == key) {
        return &(m_items[item_index].m_value);
      }
      item_index = m_items[item_index].m_next;
    }
    return nullptr;
  }
  const item_list_type& items() const { return m_items; }
  const bucket_list_type& buckets() const { return m_buckets; }
  size_type size() const { return m_items.size(); }
};

//////////////////////////////
// FirstOccurrenceHashTable //
//////////////////////////////

template <
  typename t_char = uint64_t,
  typename t_char_hash = uint64_t,
  typename t_position = uint64_t,
  typename t_allocator = std::allocator<uint8_t>
>
class FirstOccurrenceHashTable {
public:
  using char_type = t_char;
  using char_hash_type = t_char_hash;
  using position_type = t_position;
  using allocator_type = t_allocator;
private:
  static constexpr bool use_array = sizeof(char_type) == 1 && std::is_integral_v<char_type>;
  std::conditional_t<
    use_array,
    std::array<position_type, 256>,
    HashTable<char_hash_type, position_type, allocator_type>
  > m_map;
public:
  FirstOccurrenceHashTable() {
    if constexpr (use_array) { m_map.fill(std::numeric_limits<position_type>::max()); }
  }
  void insert(const char_hash_type& key, const position_type& value) {
    if constexpr (use_array) {
      assert(0 < key && key <= 256);
      m_map[key - 1] = value;
    }
    else {
      m_map.insert(key, value);
    }
  }
  position_type find(const char_hash_type& key) {
    if constexpr (use_array) {
      assert(0 < key && key <= 256);
      return m_map[key - 1];
    }
    else {
      position_type *p = m_map.find(key);
      if (p != nullptr) return *p;
      return std::numeric_limits<position_type>::max();
    }
  }
};

///////////////////////////////////////////////////////////
// FingerprintHashTableCommonPrefixNoSingletonClustering //
///////////////////////////////////////////////////////////
// FingerprintHashTable that stores matches with common prefix.
// FIND: Find the list for a given fingerprint.
// OPERATOR++ and REPORT_MATCH_LENGTH: scan through matches, use the stored
//                                     common prefixes to speed up the search,
//                                     and find where to insert the next element
//                                     if INSERT if called before next FIND.
// INSERT: If the text is a multiple of the block size, insert it at the index
//         found during OPERATOR++ and REPORT_MATCH_LENGTH.

// FingerprintHashTableCommonPrefixIterator
template <typename t_container, typename t_value>
struct FingerprintHashTableCommonPrefixNoSingletonClusteringIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using container_type = t_container;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using pointer = value_type*;
  using reference = value_type&;
  using Iterator = FingerprintHashTableCommonPrefixNoSingletonClusteringIterator;

  container_type& m_table;
  size_type* m_item_index;
  size_type m_size;
  size_type m_index;
  size_type m_node_size;
  size_type m_node_start;
  size_type m_node_offset;
  size_type m_first_match_length;
  size_type m_prefix_length;
  size_type m_best_match_length;
  size_type m_best_index;
public:
  FingerprintHashTableCommonPrefixNoSingletonClusteringIterator(container_type& table)
    : m_table(table)
    , m_item_index(nullptr)
    , m_size(0)
    , m_index(std::numeric_limits<size_type>::max())
    , m_node_size(0)
    , m_node_start(0)
    , m_node_offset(0) {}
  FingerprintHashTableCommonPrefixNoSingletonClusteringIterator(container_type& table, std::pair<size_type, size_type>* listp)
    : m_table(table)
    , m_item_index(&(listp->second))
    , m_size(listp->first)
    , m_index(0)
    , m_node_size(m_table.get_node_size(m_size))
    , m_node_start(listp->second - (m_node_size << 1))
    , m_node_offset(m_table.get_node_offset(m_size, m_node_size))
    , m_first_match_length(std::numeric_limits<size_type>::max())
    , m_prefix_length(0)
    , m_best_match_length(0)
    , m_best_index(std::numeric_limits<size_type>::max()) {}
  reference operator*() const {
    if (m_size == 1) { return *m_item_index; }
    return m_table.m_items[m_node_start + m_node_offset];
  }
  pointer operator->() const {
    if (m_size == 1) { return m_item_index; }
    return &(m_table.m_items[m_node_start + m_node_offset]);
  }
  Iterator& operator++() {
    assert(m_index < m_size);
    assert(m_table.m_state == 1 || m_table.m_state == 2 || m_table.m_state == 3);
    m_table.m_state = 2;
    if (m_index + 1 == m_size) {
      assert(m_node_size <= 2);
      m_index = std::numeric_limits<size_type>::max();
    }
    else if (m_node_offset == 2) {
      m_prefix_length = m_table.m_items[m_node_start + m_node_offset - 1];
      ++m_index;
      m_node_size = m_table.get_node_size(m_size - m_index);
      m_node_start = m_table.m_items[m_node_start] - (m_node_size << 1);
      m_node_offset = (m_node_size << 1);
    }
    else {
      m_prefix_length = m_table.m_items[m_node_start + m_node_offset - 1];
      ++m_index;
      assert(m_node_offset > 2);
      m_node_offset -= 2;
    }
    return *this;
  }
  Iterator operator++(int) { Iterator temp = *this; ++(*this); return temp; }
  bool operator==(const Iterator& other) const { return  m_index == other.m_index; }
  bool operator!=(const Iterator& other) const { return !(*this == other); }
  friend container_type;
};

// FingerprintHashTableCommonPrefixNoSingletonClustering
template <
  typename t_fingerprint = uint64_t,
  typename t_value = uint64_t,
  typename t_allocator = std::allocator<uint8_t>,
  typename t_hashtable = HashTable<t_fingerprint, std::pair<uint64_t, uint64_t>, t_allocator>,
  typename t_item = uint64_t,
  typename t_list = SpaceEfficientVector<t_item, typename std::allocator_traits<t_allocator>::template rebind_alloc<t_item>>
>
class FingerprintHashTableCommonPrefixNoSingletonClustering {
public:
  using size_type = uint64_t;
  using fingerprint_type = t_fingerprint;
  using value_type = t_value;
  using allocator_type = t_allocator;
  using hashtable_type = t_hashtable;
  using item_type = t_item;
  using list_type = t_list;
  using iterator_type = FingerprintHashTableCommonPrefixNoSingletonClusteringIterator<FingerprintHashTableCommonPrefixNoSingletonClustering, value_type>;
  friend iterator_type;
private:
  hashtable_type m_map;
  list_type m_items;

  // 0: START               -> FIND
  // 1: FIND                -> SET_MATCH_RESULTS, INSERT, FIND
  // 2: OPERATOR++          -> SET_MATCH_RESULTS, INSERT, FIND
  // 3: SET_MATCH_RESULTS   -> OPERATOR++, INSERT
  // 4: INSERT              -> FIND
  uint64_t m_state = 0;

public:
  FingerprintHashTableCommonPrefixNoSingletonClustering() = default;

  // Insert the item after it.m_prev_item_index and before it.m_item_index.
  void insert(const fingerprint_type& key, const value_type& index, iterator_type& it) {
    assert(m_state == 1 || m_state == 2 || m_state == 3);
    m_state = 4;
    auto *listp = m_map.find(key);
    // If match list does not exist, create one.
    if (listp == nullptr) {
      listp = m_map.insert(key, std::make_pair(1UL, index));
    }
    // Otherwise, add to existing match list.
    else {
      // assert(it.m_first_match_length != std::numeric_limits<size_type>::max());
      size_type list_size = listp->first;
      if (list_size == 1) {
        // Add space for node.
        size_type new_node_size = 2;
        m_items.resize(m_items.size() + (new_node_size << 1) - 1);
        // Get node start.
        size_type new_node_start = m_items.size() - 1 - (new_node_size << 1);
        size_type new_node_offset = 4;
        // Set to first match.
        m_items[new_node_start + new_node_offset - 2] = listp->second;
        listp->second = m_items.size() - 1;
        // Add index.
        m_items[new_node_start + new_node_offset - 1] = it.m_first_match_length;
        m_items[new_node_start + new_node_offset] = index;
      }
      // If the list size is 2 less than a power of 2, add a new node.
      else if (((list_size + 1) & (list_size + 2)) == 0) {
        // Add space for node.
        size_type new_node_size = get_node_size(list_size + 1);
        m_items.resize(m_items.size() + (new_node_size << 1) + 1);
        // Get node start.
        size_type new_node_start = m_items.size() - 1 - (new_node_size << 1);
        size_type new_node_offset = 2;
        // Point to previous node.
        m_items[new_node_start] = listp->second;
        listp->second = m_items.size() - 1;
        // Add index.
        m_items[new_node_start + new_node_offset - 1] = it.m_first_match_length;
        m_items[new_node_start + new_node_offset] = index;
      }
      // Otherwise, add to existing node.
      else {
        size_type node_size = get_node_size(list_size);
        size_type node_start = listp->second - (node_size << 1);
        size_type node_offset = get_node_offset(list_size + 1, node_size);
        m_items[node_start + node_offset - 1] = it.m_first_match_length;
        m_items[node_start + node_offset] = index;
      }
      listp->first += 1;

      // Cluster matches by common prefix.
      {
        size_type list_size = listp->first;
        size_type best_index = it.m_best_index;
        size_type best_match_length = it.m_best_match_length;
        bool clustering_enabled = best_index != std::numeric_limits<size_type>::max()
            && lz77_approx::most_significant_bit(best_match_length) > best_index;
        if (clustering_enabled) {
          // Gradually swap the new match into place.
          size_type node_size = get_node_size(list_size);
          size_type node_start = listp->second - (node_size << 1);
          size_type node_offset = get_node_offset(list_size, node_size);
          m_items[node_start + node_offset - 1] = best_match_length;
          for (size_type i = 0; i < best_index; ++i) {
            assert(node_offset >= 2);
            // Swap within a node.
            if (node_offset > 2) {
              std::swap(m_items[node_start + node_offset - 1], m_items[node_start + node_offset - 3]);
              std::swap(m_items[node_start + node_offset], m_items[node_start + node_offset - 2]);
              node_offset -= 2;
            }
            // Swap over node boundary.
            else if (node_offset == 2) {
              assert(node_size > 2);
              size_type new_node_size = node_size >> 1;
              size_type new_node_start = m_items[node_start] - (new_node_size << 1);
              size_type new_node_offset = new_node_size << 1;
              std::swap(m_items[node_start + node_offset - 1], m_items[new_node_start + new_node_offset - 1]);
              std::swap(m_items[node_start + node_offset], m_items[new_node_start + new_node_offset]);
              node_size = new_node_size;
              node_start = new_node_start;
              node_offset = new_node_offset;
            }
          }
        }
      }
    }
  }

  iterator_type find(const fingerprint_type& key) {
    assert(m_state == 0 || m_state == 1 || m_state == 2 || m_state == 3 || m_state == 4);
    m_state = 1;
    // Return iterator to list start.
    auto *list_start = m_map.find(key);
    if (list_start != nullptr) {
      return iterator_type(*this, list_start);
    }
    return iterator_type(*this);
  }

  void set_match_results(
      iterator_type& it,
      size_type match_length,
      bool match_lex_smaller
  ) {
    assert(m_state == 1 || m_state == 2);
    m_state = 3;
    if (it.m_first_match_length == std::numeric_limits<size_type>::max()) {
      it.m_first_match_length = match_length;
    }
    if (match_length > it.m_best_match_length) {
      it.m_best_match_length = match_length;
      it.m_best_index = it.m_index;
    }
  }

  size_type get_prefix_length(const iterator_type& it) const { return it.m_prefix_length; }

  iterator_type end() { return iterator_type(*this); }
  size_type list_size(const fingerprint_type& key) { return 0; }
  size_type size() const { return m_items.size(); }
  const auto& buckets() const { return m_map.items(); }
  size_type get_node_size(size_type list_size) { return 1ULL << (63 -__builtin_clzll(static_cast<uint64_t>(list_size + 1))); }
  size_type get_node_offset(size_type list_size, size_type node_size) { return (((list_size + 1) & (node_size - 1)) << 1) + 2; }

};

template <typename t_fingerprint, typename t_size>
using FingerprintHashTableChosen = FingerprintHashTableCommonPrefixNoSingletonClustering<t_fingerprint, t_size>;

} // lz77_approx

#endif // __LZ77_APPROX__HASH_TABLE_HPP_INCLUDED
