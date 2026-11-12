#ifndef __LZ77_APPROX__IO_HPP_INCLUDED
#define __LZ77_APPROX__IO_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

#include "karp_rabin_fingerprint.hpp"
#include "space_efficient_vector.hpp"
#include "utils.hpp"

namespace lz77_approx {

// has_io_flush.
template<typename T, typename = void> struct has_io_flush : std::false_type {};
template<typename T> struct has_io_flush<T, std::void_t<decltype(std::declval<T>().flush())>> : std::true_type {};

/////////////
// Phrase. //
/////////////

template <typename t_char = uint8_t, typename t_size = uint64_t>
class Phrase {

public:
  using char_type = t_char;
  using size_type = t_size;

  union {
    char_type chr;
    size_type start;
  };
  size_type length;

  Phrase(): start(), length() {};
  Phrase(char_type c): chr(c), length(0) {};
  Phrase(size_type s, size_type l): start(s), length(l) {};

  Phrase(const Phrase& other) {
    length = other.length;
    if (length) { start = other.start; } else { chr = other.chr; }
  }
  Phrase(Phrase&& other) noexcept {
    length = std::move(other.length);
    if (length) { start = std::move(other.start); } else { chr = std::move(other.chr); }
  }

  Phrase& operator=(const Phrase& other) {
    if (this != &other) {
      length = other.length;
      if (length) { start = other.start; } else { chr = other.chr; }
    }
    return *this;
  }

  Phrase& operator=(Phrase&& other) noexcept {
    if (this != &other) {
      length = std::move(other.length);
      if (length) { start = std::move(other.start); } else { chr = std::move(other.chr); }
    }
    return *this;
  }

  bool is_literal() const { return length == 0; }

  size_type serialize(std::ostream& ofs) const {
    ofs.write(reinterpret_cast<const char*>(&start), sizeof(size_type));
    ofs.write(reinterpret_cast<const char*>(&length), sizeof(size_type));
    return sizeof(size_type) + sizeof(size_type);
  }

  void load(std::istream& ifs) {
    ifs.read(reinterpret_cast<char*>(&start), sizeof(size_type));
    ifs.read(reinterpret_cast<char*>(&length), sizeof(size_type));
  }

};

// ////////////////
// // Input Base //
// ////////////////
// 
// template <
//   typename t_char,
//   typename t_size,
//   typename t_iterator,
//   typename t_fingerprint_manager = FingerprintManager>
// class InputBase {
// public:
// 
//   using char_type = t_char;
//   using size_type = t_size;
//   using iterator_type = t_iterator;
//   using fingerprint_manager_type = t_fingerprint_manager;
//   using fingerprint_type = typename fingerprint_manager_type::fingerprint_type;
//   using phrase_type = Phrase<char_type, size_type>;
// 
//   virtual ~InputBase() {}
//   virtual const char_type& operator[](size_type i) const = 0;
//   virtual size_type size() = 0;
//   virtual iterator_type begin() = 0; 
//   virtual iterator_type end() = 0;
//   virtual fingerprint_type get_fingerprint(size_type text_i, size_type length) = 0;
//   virtual fingerprint_type get_fingerprint(fingerprint_type fingerprint, size_type length, char_type old_char, char_type new_char) = 0;

// };
// 
// /////////////////
// // Output Base //
// /////////////////
// 
// template <
//   typename t_value,
//   typename t_size>
// class OutputBase {
// public:
// 
//   using value_type = t_value;
//   using size_type = t_size;
// 
//   virtual ~OutputBase() {}
//   virtual void push_back(const value_type& value) = 0;
//   virtual void push_back(value_type&& value) = 0;
//   virtual void resize(size_type size, const value_type& fill = value_type()) = 0;
//   virtual size_type size() const = 0;
// 
// };

///////////////////////////
// File Stream Formatter //
///////////////////////////

template <typename T>
class FileStreamFormatter {
public:

  static void read(std::istream& ifs, T& value) {
    if constexpr (has_load<T>::value) {
      value.load(ifs);
    }
    else {
      ifs >> value;
    }
  }

  static void write(std::ostream& ofs, const T& value) {
    if constexpr (has_serialize<T>::value) {
      value.serialize(ofs);
    }
    else {
      ofs << value << std::endl;
    }
  }

};

///////////////////
// Common Inputs //
///////////////////

// Raw string input.
template <
  typename t_char = std::uint8_t, 
  typename t_size = std::uint64_t,
  typename t_fingerprint_manager = FingerprintManager
>
class InputRawString {
public:
  using char_type = t_char;
  using size_type = t_size;
  using iterator = char_type *;
  using fingerprint_manager_type = t_fingerprint_manager;
  using fingerprint_type = typename fingerprint_manager_type::fingerprint_type;
  using phrase_type = Phrase<char_type, size_type>;
private:
  char_type *m_text;
  size_type m_length;
public:
  explicit InputRawString(char_type *text, size_type length) : m_text(text), m_length(length) {}
  const char_type& operator[](size_type i) const { return m_text[i]; }
  char_type& operator[](size_type i) { return m_text[i]; }
  size_type size() { return m_length; }
  iterator begin() { return m_text; }
  iterator end() { return m_text + m_length; }
  fingerprint_type get_fingerprint(size_type text_i, size_type length) {
    return fingerprint_manager_type::get_fingerprint(m_text + text_i, m_text + text_i + length);
  }
  fingerprint_type get_fingerprint(fingerprint_type fingerprint, size_type length, char_type old_char, char_type new_char) {
    return fingerprint_manager_type::get_fingerprint(fingerprint, length, old_char, new_char);
  }
};

// Vector input.
template <
  typename t_vector = std::vector<uint8_t>,
  typename t_fingerprint_manager = FingerprintManager
>
class InputVector {
public:
  using vector_type = t_vector;
  using char_type = typename vector_type::value_type;
  using size_type = typename vector_type::size_type;
  using const_iterator = typename vector_type::const_iterator;
  using fingerprint_manager_type = t_fingerprint_manager;
  using fingerprint_type = typename fingerprint_manager_type::fingerprint_type;
  using phrase_type = Phrase<char_type, size_type>;
private:
  const vector_type& m_vec;
public:
  explicit InputVector(const vector_type& vec) : m_vec(vec) {}
  const char_type& operator[](size_type i) const { return m_vec[i]; }
  size_type size() { return m_vec.size(); }
  const_iterator begin() { return m_vec.begin(); }
  const_iterator end() { return m_vec.end(); }
  fingerprint_type get_fingerprint(size_type text_i, size_type length) {
    return fingerprint_manager_type::get_fingerprint(m_vec.begin() + text_i, m_vec.begin() + text_i + length);
  }
  fingerprint_type get_fingerprint(fingerprint_type fingerprint, size_type length, char_type old_char, char_type new_char) {
    return fingerprint_manager_type::get_fingerprint(fingerprint, length, old_char, new_char);
  }
};

// File input.
template <
  typename t_value = uint8_t,
  typename t_allocator = std::allocator<t_value>,
  typename t_fingerprint_manager = FingerprintManager
>
class InputFile {
public:
  using allocator_type = t_allocator;
  using vector_type = std::vector<t_value, allocator_type>;
  using fingerprint_manager_type = t_fingerprint_manager;
  using fingerprint_type = typename fingerprint_manager_type::fingerprint_type;
  using char_type = typename vector_type::value_type;
  using size_type = typename vector_type::size_type;
  using iterator = typename vector_type::iterator;
  using phrase_type = Phrase<char_type, size_type>;
private:
  vector_type m_text;
public:
  explicit InputFile(std::string file, size_type prefix_length = std::numeric_limits<size_type>::max()) {
    std::ifstream is(file, std::ios::binary | std::ios::ate);
    size_type text_length = is.tellg();
    text_length = std::min(text_length, prefix_length);
    is.seekg(0, std::ios::beg);
    m_text = vector_type(text_length);
    is.read(reinterpret_cast<char*>(m_text.data()), text_length);
    is.close();
  }
  const char_type& operator[](size_type i) const { return m_text[i]; }
  char_type& operator[](size_type i) { return m_text[i]; }
  size_type size() { return m_text.size(); }
  iterator begin() { return m_text.begin(); }
  iterator end() { return m_text.end(); }
  fingerprint_type get_fingerprint(size_type text_i, size_type length) {
    return fingerprint_manager_type::get_fingerprint(m_text.begin() + text_i, m_text.begin() + text_i + length);
  }
  fingerprint_type get_fingerprint(fingerprint_type fingerprint, size_type length, char_type old_char, char_type new_char) {
    return fingerprint_manager_type::get_fingerprint(fingerprint, length, old_char, new_char);
  }

};

////////////////////
// Common Outputs //
////////////////////

// Vector output.
template <
  typename t_value = uint8_t,
  typename t_allocator = std::allocator<t_value>
>
using OutputVector = std::vector<t_value, t_allocator>;

// Space-efficient vector output.
template <
  typename t_value = uint8_t,
  typename t_allocator = std::allocator<t_value>
>
using OutputSpaceEfficientVector = SpaceEfficientVector<t_value, t_allocator>;

// File output.
template <
  typename t_value = uint8_t,
  typename t_size = uint64_t,
  typename t_allocator = std::allocator<t_value>,
  typename t_formatter = FileStreamFormatter<t_value>
>
class OutputFile {
public:
  using value_type = t_value;
  using size_type = t_size;
  using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_value>;
  using formatter_type = t_formatter;
  using vector_type = SpaceEfficientVector<value_type, allocator_type>;
  using iterator = typename vector_type::iterator;
  using const_iterator = typename vector_type::const_iterator;
private:
  std::ofstream m_ofs;
  vector_type m_vec;
public:
  OutputFile(std::string output_file_name)
    : m_ofs(output_file_name) {}
  const value_type& operator[] (const size_type& i) const { return m_vec[i]; }
  value_type& operator[] (const size_type& i) { return m_vec[i]; }
  auto begin() { return m_vec.begin(); }
  auto begin() const { return m_vec.begin(); }
  auto end() { return m_vec.end(); }
  auto end() const { return m_vec.end(); }
  void push_back(const value_type& value) { m_vec.push_back(value); }
  void push_back(value_type&& value) { m_vec.push_back(value); }
  void resize(size_type size, const value_type& fill = value_type()) { m_vec.resize(size); }
  size_type size() const { return m_vec.size(); }
  void flush() {
    for (uint64_t i = 0; i < m_vec.size(); ++i) {
      formatter_type::write(m_ofs, m_vec[i]);
    }
    m_ofs.flush();
  }
  // Additional functions.
  const vector_type& as_vector() const { return m_vec; }
};

// File stream output.
// Guarantees to hoard a certain number of values.
template <
  typename t_value = uint8_t,
  typename t_size = uint64_t,
  typename t_allocator = std::allocator<t_value>,
  typename t_formatter = FileStreamFormatter<t_value>
>
class OutputFileStream {
public:

  using value_type = t_value;
  using size_type = t_size;
  using allocator_type = typename std::allocator_traits<t_allocator>::template rebind_alloc<t_value>;
  using formatter_type = t_formatter;

private:

  size_type m_hoard_min_log;
  size_type m_hoard_min_size;
  size_type m_hoard_min_mask;
  size_type m_size;
  size_type m_start;
  size_type m_offset;
  size_type m_committed;
  std::ofstream m_ofs;
  std::vector<value_type, allocator_type> m_buffer;

public:

  // Constuct two buffers of size 1 << hoard_min_log values.
  // We guarantee that the last 1 << hoard_min_log values are
  // accessible to the user and not added to the file.
  OutputFileStream(
      std::string output_file_name,
      size_type hoard_min_log
  ) : m_hoard_min_log(hoard_min_log),
      m_hoard_min_size(1 << m_hoard_min_log),
      m_hoard_min_mask(m_hoard_min_size - 1),
      m_size(0),
      m_start(m_hoard_min_size),
      m_offset(m_hoard_min_size),
      m_committed(0),
      m_ofs(output_file_name),
      m_buffer(m_hoard_min_size << 1) {}

  ~OutputFileStream() {
    flush();
  }

  void push_back(const value_type& value) {
    prepare_to_increment_size();
    m_buffer[m_start + m_offset] = value;
    ++m_offset;
    ++m_size;
  }

  void push_back(value_type&& value) {
    prepare_to_increment_size();
    m_buffer[m_start + m_offset] = value;
    ++m_offset;
    ++m_size;
  }

  // If size is smaller than m_size by more than m_hoard_min_size,
  // it is possible that an error is thrown, because the data has
  // already been written to the file.
  void resize(size_type size, const value_type& fill = value_type()) {
    // Handle size == m_size.
    if (size == m_size) {
      return;
    }
    // Handle size > m_size.
    else if (size > m_size) {
      // TODO: reimplement this faster.
      for (uint64_t i = m_size; i < size; ++i) {
        push_back(fill);
      }
    }
    // Handle size < m_size.
    else if (size < m_committed) {
      std::cerr << "Error: failed to resize output to size "
                << size 
                << ", because "
                << m_committed
                << " elements have been written to file."
                << std::endl;
      std::exit(EXIT_FAILURE);
    }
    else {
      size_type delta = m_size - size;
      if (delta < m_offset) {
        m_offset = m_offset - delta;
      }
      else if (delta < m_hoard_min_size + m_offset) {
        m_start = m_hoard_min_size - m_start;
        m_offset = m_hoard_min_size + m_offset - delta;
      }
      else if (delta == m_hoard_min_size + m_offset) {
        m_offset = m_hoard_min_size;
      }
    }
    m_size = size;
  }

  size_type size() const { return m_size; }
  void flush() {

    // Items to commit.
    size_type items_to_commit = m_size - m_committed;

    // Flush data in the buffer opposite the current position.
    if (items_to_commit > m_hoard_min_size) {
      size_type commit_offset = m_hoard_min_size + m_offset - items_to_commit;
      for (size_type i = commit_offset; i < m_hoard_min_size; ++i) {
        formatter_type::write(m_ofs, m_buffer[m_hoard_min_size - m_start + i]);
      }
      m_committed += m_hoard_min_size - commit_offset;
      items_to_commit = m_size - m_committed;
    }

    // Flush data in same buffer as the current position.
    for (size_type i = m_offset - items_to_commit; i < m_offset; ++i) {
      formatter_type::write(m_ofs, m_buffer[m_start + i]);
    }

    // Update committed.
    m_committed = m_size;

    // Flush output file stream.
    m_ofs.flush();

  }

  const auto& buffer() const { return m_buffer; }

private:

  void prepare_to_increment_size() {

    // If the current buffer is filled.
    if (m_offset == m_hoard_min_size) {
      // Move to the other buffer.
      m_start = m_hoard_min_size - m_start;
      m_offset = 0;
      // If this buffer has content, flush it.
      size_type items_to_commit = m_size - m_committed;
      if (items_to_commit > m_hoard_min_size) {
        size_type commit_offset = m_buffer.size() - items_to_commit ;
        for (size_type i = commit_offset; i < m_hoard_min_size; ++i) {
          formatter_type::write(m_ofs, m_buffer[m_start + i]);
        }
        // Update committed end.
        m_committed += m_hoard_min_size - commit_offset;
      }
    }
  }

};

} // lz77_approx

#endif // __LZ77_APPROX__IO_HPP_INCLUDED
