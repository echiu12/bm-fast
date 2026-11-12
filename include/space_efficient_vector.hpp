#ifndef __LZ77_APPROX__SPACE_EFFICIENT_VECTOR_HPP_INCLUDED
#define __LZ77_APPROX__SPACE_EFFICIENT_VECTOR_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>

#include "utils.hpp"

namespace lz77_approx {

// Adjusted from space_efficient_vector.hpp by Dominik Kempa.
template<typename t_value, typename t_allocator = std::allocator<t_value>>
class SpaceEfficientVector {
public:

  using value_type = t_value;
  using size_type = uint64_t;
  using allocator_type = t_allocator;

private:

  using ptr_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<value_type*>;
  using value_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<value_type>;

  using ptr_trait_type = std::allocator_traits<ptr_allocator_type>;
  using value_trait_type = std::allocator_traits<value_allocator_type>;

  ptr_allocator_type ptr_allocator;
  value_allocator_type value_allocator;

  // Define members.
  value_type **m_blocks;
  size_type m_block_size_log;
  size_type m_block_size_mask;
  size_type m_block_size;
  size_type m_allocated_blocks;
  size_type m_cur_block_id;
  size_type m_cur_block_filled;
  size_type m_size;

private:

  // max_blocks must be power of 2.
  static const size_type max_blocks = 32;

public:

  SpaceEfficientVector() {
    m_size = 0;
    m_block_size_log = 0;
    m_block_size_mask = 0;
    m_block_size = 1;
    m_allocated_blocks = 1;
    m_cur_block_filled = 0;
    m_cur_block_id = 0;
    m_blocks = ptr_trait_type::allocate(ptr_allocator, max_blocks);
    m_blocks[0] = value_trait_type::allocate(value_allocator, m_block_size);
  }

  ~SpaceEfficientVector() {
    clear();
    deallocate_all();
  }

  // Note: new vector may differ in block size, allocated blocks, etc...
  SpaceEfficientVector(const SpaceEfficientVector& other) : SpaceEfficientVector() {
    if (this != &other) {

      // Allocate all necessary blocks.
      this->expand(other.m_size);

      // Copy each element.
      // Note: we use the fact that the new vector must have the smaller block size.
      // Note: we also use the fact that the new vector block size must be a factor of the other.
      for (size_type i = 0, block_id = 0; i < other.size(); i += m_block_size, ++block_id) {
        size_type length = std::min(m_block_size, other.size() - i);
        for (size_type j = 0; j < length; ++j) {
          value_trait_type::construct(value_allocator, m_blocks[block_id] + j, other[i + j]);
        }
      }
      m_size = other.m_size;

    }
  }

  SpaceEfficientVector(SpaceEfficientVector&& other) {
    if (this != &other) {

      // Pilfer.
      m_size = other.m_size;
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_filled = other.m_cur_block_filled;
      m_cur_block_id = other.m_cur_block_id;
      m_blocks = other.m_blocks;

      // Put other into a valid state.
      other.m_size = 0;
      other.m_block_size_log = 0;
      other.m_block_size_mask = 0;
      other.m_block_size = 1;
      other.m_allocated_blocks = 1;
      other.m_cur_block_filled = 0;
      other.m_cur_block_id = 0;
      other.m_blocks = ptr_trait_type::allocate(ptr_allocator, max_blocks);
      other.m_blocks[0] = value_trait_type::allocate(value_allocator, m_block_size);

    }
  }

  SpaceEfficientVector(std::initializer_list<value_type> list) : SpaceEfficientVector() {
    resize(list.size());
    size_type i = 0;
    for (const auto& value : list) {
      this->operator[](i) = value;
      ++i;
    }
  }

  SpaceEfficientVector& operator=(const SpaceEfficientVector& other) {
    if (this != &other) {

      // Destroy old values (but not necessary allocated blocks).
      clear();

      // Allocate all necessary blocks.
      expand(other.m_size);

      // Copy construct each element.
      for (uint64_t i = 0; i < other.size(); ++i) {
        const size_type block_id = (i >> m_block_size_log);
        const size_type block_offset = (i & m_block_size_mask);
        value_trait_type::construct(value_allocator, m_blocks[block_id] + block_offset, other[i]);
      }
      m_size = other.m_size;
      m_cur_block_id = (m_size >> m_block_size_log);
      m_cur_block_filled = (m_size & m_block_size_mask);

    }

    return *this;
  }

  SpaceEfficientVector& operator=(SpaceEfficientVector&& other) {
    if (this != &other) {

      // Destroy old values, blocks, and block pointers.
      clear();
      deallocate_all();

      // Pilfer.
      m_size = other.m_size;
      m_block_size_log = other.m_block_size_log;
      m_block_size_mask = other.m_block_size_mask;
      m_block_size = other.m_block_size;
      m_allocated_blocks = other.m_allocated_blocks;
      m_cur_block_filled = other.m_cur_block_filled;
      m_cur_block_id = other.m_cur_block_id;
      m_blocks = other.m_blocks;

      // Put other into a valid state.
      other.m_size = 0;
      other.m_block_size_log = 0;
      other.m_block_size_mask = 0;
      other.m_block_size = 1;
      other.m_allocated_blocks = 1;
      other.m_cur_block_filled = 0;
      other.m_cur_block_id = 0;
      other.m_blocks = ptr_trait_type::allocate(ptr_allocator, max_blocks);
      other.m_blocks[0] = value_trait_type::allocate(value_allocator, m_block_size);

    }
    return *this;
  }

  inline size_type size() const {
    return m_size;
  }

  inline bool empty() const {
    return m_size == 0;
  }

  // Destroy all items.
  inline void clear() {
    for (size_type i = 0; i < m_cur_block_id; ++i) {
      for (size_type j = 0; j < m_block_size; ++j) {
        value_trait_type::destroy(value_allocator, m_blocks[i] + j);
      }
    }
    for (size_type j = 0; j < m_cur_block_filled; ++j) {
      value_trait_type::destroy(value_allocator, m_blocks[m_cur_block_id] + j);
    }
    m_size = 0;
    m_cur_block_id = 0;
    m_cur_block_filled = 0;
  }

  // Deallocate blocks and list of block pointers.
  inline void deallocate_all() {
    for (size_type i = 0; i < m_allocated_blocks; ++i) {
      value_trait_type::deallocate(value_allocator, m_blocks[i], m_block_size);
    }
    ptr_trait_type::deallocate(ptr_allocator, m_blocks, max_blocks);
  }

  inline void pop_back() {
    assert(m_size > 0);
    --m_size;
    --m_cur_block_filled;
    value_trait_type::destroy(value_allocator, &m_blocks[m_cur_block_id][m_cur_block_filled]);
    if (m_cur_block_filled == 0 && m_cur_block_id > 0) {
      --m_cur_block_id;
      m_cur_block_filled = m_block_size;
    }
  }

  inline void push_back(const value_type& value) {

    // Ensure that the space is available.
    enlarge_by_one();

    // Add value.
    value_trait_type::construct(value_allocator, m_blocks[m_cur_block_id] + m_cur_block_filled, value);
    ++m_cur_block_filled;
    ++m_size;

  }

  inline void push_back(value_type&& value) {

    // Ensure that the space is available.
    enlarge_by_one();

    // Add value.
    value_trait_type::construct(value_allocator, m_blocks[m_cur_block_id] + m_cur_block_filled, std::move(value));
    ++m_cur_block_filled;
    ++m_size;

  }

  template <class... Args>
  inline value_type& emplace_back(Args&&... args) {

    // Ensure that the space is available.
    enlarge_by_one();

    // Add value.
    value_trait_type::construct(value_allocator, m_blocks[m_cur_block_id] + m_cur_block_filled, args...);
    ++m_cur_block_filled;
    ++m_size;

    return back();

  }

  // Increase block size until it can handle size elements without reallocation.
  // If there are unused allocated blocks (e.g. from pop_back), they are removed.
  //
  void reserve(const size_type& size) {

    // Compute the new block size.
    //    size      min_block_size    new_block_size_log
    //   0 -  32 -> 0              -> 0
    //  33 -  64 -> 1              -> 1
    //  65 - 128 -> 2 or 3         -> 2
    // 129 - 256 -> 4 to 7         -> 3
    const size_type min_block_size = size ? (size - 1) / max_blocks : 0;
    const size_type new_block_size_log = min_block_size ? (64 - __builtin_clzll(min_block_size)) : 0;

    // If necessary, enlarge the block size.
    if (new_block_size_log > m_block_size_log) {

      // Remove unused blocks.
      size_type occupied_blocks = (m_size + m_block_size - 1) / m_block_size;
      for (size_type i = occupied_blocks; i < m_allocated_blocks; ++i) {
        value_trait_type::deallocate(value_allocator, m_blocks[i], m_block_size);
      }

      // Realloc remaining blocks.
      const size_type new_block_size = 1 << new_block_size_log;
      const size_type new_allocated_blocks = m_size ? (m_size + new_block_size - 1) / new_block_size : 1;
      const size_type blocks_per_new_block = 1 << (new_block_size_log - m_block_size_log);
      for (size_type new_block_id = 0, block_id = 0; new_block_id < new_allocated_blocks; ++new_block_id, block_id += blocks_per_new_block) {
        value_type *new_block = value_trait_type::allocate(value_allocator, new_block_size);
        value_type *tmp = new_block;
        for (size_type i = block_id, end = i + blocks_per_new_block; i < end && i < occupied_blocks; ++i) {
          for (size_type j = 0; j < m_block_size; ++j) {
            value_trait_type::construct(value_allocator, tmp + j, std::move(m_blocks[i][j]));
          }
          value_trait_type::deallocate(value_allocator, m_blocks[i], m_block_size);
          tmp += m_block_size;
        }

        m_blocks[new_block_id] = new_block;
      }

      // Update members (m_size unchanged)
      m_block_size_log = new_block_size_log;
      m_block_size = new_block_size;
      m_block_size_mask = new_block_size - 1;
      m_allocated_blocks = new_allocated_blocks;

      m_cur_block_id = m_size ? (m_size - 1) / m_block_size : 0;
      m_cur_block_filled = m_size - m_cur_block_id * m_block_size;

    }
  }

  // Reserve, then allocate blocks until it can handle size elements without allocating additional blocks.
  // Does not modify m_size, m_cur_block_id, and m_cur_block_filled.
  void expand(const size_type& size) {
    reserve(size);

    // Allocate as many blocks as necessary.
    size_type new_allocated_blocks = size ? (size + m_block_size - 1) / m_block_size : 1;
    for (size_type i = m_allocated_blocks; i < new_allocated_blocks; ++i) {
      m_blocks[i] = value_trait_type::allocate(value_allocator, m_block_size);
    }
    m_allocated_blocks = new_allocated_blocks;
  }

  // Expand, then fill blocks with initial value.
  inline void resize(const size_type& size, const value_type &initial_val = value_type()) {
    if (size > m_size) {
      expand(size);

      // Initialize elements.
      size_type block_id = m_cur_block_id;
      size_type block_filled = m_cur_block_filled;
      uint64_t block_available = m_block_size - m_cur_block_filled;
      uint64_t remaining_elements = size - m_size;
      while (remaining_elements > 0) {
        value_type *start = m_blocks[block_id] + block_filled;
        uint64_t length = std::min(block_available, remaining_elements);
        for (uint64_t i = 0; i < length; ++i) {
          value_trait_type::construct(value_allocator, start + i, initial_val);
        }
        block_available = m_block_size;
        remaining_elements -= length;
        ++block_id;
        block_filled = 0;
      }

    }
    else if (size < m_size) {

      // Remove elements.
      size_type block_id = size ? (size - 1) / m_block_size : 0;
      size_type block_filled = size - block_id * m_block_size;
      size_type block_available = m_block_size - block_filled;
      size_type remaining_elements = m_size - size;
      while (remaining_elements > 0) {
        value_type *start = m_blocks[block_id] + block_filled;
        uint64_t length = std::min(block_available, remaining_elements);
        for (uint64_t i = 0; i < length; ++i) {
          value_trait_type::destroy(value_allocator, start + i);
        }
        block_available = m_block_size;
        remaining_elements -= length;
        ++block_id;
        block_filled = 0;
      }

    }

    // Update.
    m_size = size;
    m_cur_block_id = m_size ? (m_size - 1) / m_block_size : 0;
    m_cur_block_filled = m_size - m_cur_block_id * m_block_size;
  }

  inline value_type& front() {
    return m_blocks[0][0];
  }

  inline const value_type& front() const {
    return m_blocks[0][0];
  }

  inline value_type& back() {
    return m_blocks[m_cur_block_id][m_cur_block_filled - 1];
  }

  inline const value_type& back() const {
    return m_blocks[m_cur_block_id][m_cur_block_filled - 1];
  }
  
  inline const value_type& operator[] (const size_type& i) const {
    assert(m_size > i);
    const size_type block_id = (i >> m_block_size_log);
    const size_type block_offset = (i & m_block_size_mask);
    return m_blocks[block_id][block_offset];
  }

  inline value_type& operator[] (const size_type& i) {
    assert(m_size > i);
    const size_type block_id = (i >> m_block_size_log);
    const size_type block_offset = (i & m_block_size_mask);
    return m_blocks[block_id][block_offset];
  }

  template <typename T, bool isConst = false>
  class Iterator {
  public:

    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::remove_const_t<T>;
    using pointer = std::conditional_t<isConst, const value_type*, value_type*>;
    using reference = std::conditional_t<isConst, const value_type&, value_type&>;
    using difference_type = std::ptrdiff_t;

    using container_type = SpaceEfficientVector<value_type, allocator_type>;
    using container_pointer_type = typename std::conditional_t<isConst, const container_type*, container_type*>;
    using index_type = typename container_type::size_type;

  private:

    container_pointer_type m_v;
    index_type m_index;

  public:

    Iterator(container_pointer_type v, index_type index = 0) { m_v = v; m_index = index; }
    Iterator(const Iterator<T, false>& other) : m_v(other.m_v), m_index(other.m_index) {}
    reference operator[](difference_type i) const { return (*m_v)[m_index + i]; }
    reference operator*() const { return (*this)[0]; }
    pointer operator->() const { return &((*this)[0]); }
    Iterator& operator++() { ++m_index; return *this; }
    Iterator& operator--() { --m_index; return *this; }
    Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
    Iterator operator--(int) { Iterator tmp = *this; --(*this); return tmp; }
    Iterator operator+=(difference_type n) { m_index += n; return *this; }
    Iterator operator-=(difference_type n) { m_index -= n; return *this; }
    friend Iterator operator+(Iterator it, difference_type n) { return it += n; }
    friend Iterator operator-(Iterator it, difference_type n) { return it -= n; }
    difference_type operator+(const Iterator& other) const { return this->m_index + other.m_index; }
    difference_type operator-(const Iterator& other) const { return this->m_index - other.m_index; }
    bool operator==(const Iterator& other) const { return this->m_index == other.m_index; }
    bool operator!=(const Iterator& other) const { return this->m_index != other.m_index; }
    bool operator<(const Iterator& other) const { return this->m_index < other.m_index; }
    bool operator>(const Iterator& other) const { return this->m_index > other.m_index; }
    bool operator<=(const Iterator& other) const { return this->m_index <= other.m_index; }
    bool operator>=(const Iterator& other) const { return this->m_index >= other.m_index; }

  };
  
  using iterator = Iterator<value_type, false>;
  using const_iterator = Iterator<value_type, true>;

  inline auto begin() { return Iterator<value_type, false>(this, 0); }
  inline auto end() { return Iterator<value_type, false>(this, m_size); }
  inline auto begin() const { return Iterator<const value_type, true>(this, 0); }
  inline auto end() const { return Iterator<const value_type, true>(this, m_size); }
  inline auto cbegin() const { return Iterator<const value_type, true>(this, 0); }
  inline auto cend() const { return Iterator<const value_type, true>(this, m_size); }

private:

  void enlarge_by_one() {
    // If the block is full, add a new block.
    if (m_cur_block_filled == m_block_size) {
      // If no more blocks can be added, enlarge blocks first.
      if (m_cur_block_id + 1 == max_blocks) {
        // Enlarge block to double the original size.
        const size_type new_block_size = m_block_size * 2;
        for (size_type block_id = 0; block_id < m_allocated_blocks; block_id += 2) {
          value_type *new_block = value_trait_type::allocate(value_allocator, new_block_size);
          for (size_type i = 0; i < m_block_size; ++i) {
            value_trait_type::construct(value_allocator, new_block + i, std::move(m_blocks[block_id][i]));
            value_trait_type::construct(value_allocator, new_block + m_block_size + i, std::move(m_blocks[block_id + 1][i]));
          }
          value_trait_type::deallocate(value_allocator, m_blocks[block_id], m_block_size);
          value_trait_type::deallocate(value_allocator, m_blocks[block_id + 1], m_block_size);
          m_blocks[block_id / 2] = new_block;
        }
        m_allocated_blocks = max_blocks / 2;
        m_block_size = new_block_size;
        m_block_size_mask = new_block_size - 1;
        ++m_block_size_log;
        m_cur_block_id = m_allocated_blocks - 1;
        m_cur_block_filled = new_block_size;
      }
      // Add a new block.
      ++m_cur_block_id;
      m_cur_block_filled = 0;
      if (m_cur_block_id == m_allocated_blocks) {
        ++m_allocated_blocks;
        m_blocks[m_cur_block_id] = value_trait_type::allocate(value_allocator, m_block_size);
      }
    }
  }

};

} // lz77_approx

#endif // __LZ77_APPROX__SPACE_EFFICIENT_VECTOR_HPP_INCLUDED
