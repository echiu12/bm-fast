#ifndef __LZ77_APPROX__BM_FAST_HPP_INCLUDED
#define __LZ77_APPROX__BM_FAST_HPP_INCLUDED

#include "hash_table.hpp"
#include "io.hpp"
#include "karp_rabin_fingerprint.hpp"
#include "space_efficient_vector.hpp"
#include "utils.hpp"

namespace lz77_approx {

// BMFast.
template <
  typename t_input,
  typename t_output = OutputSpaceEfficientVector<typename t_input::phrase_type>,
  typename t_hashtable = FingerprintHashTableChosen<typename t_input::fingerprint_type, typename t_input::size_type>
>
class BMFast {

  static_assert(is_fingerprint_hash_table<t_hashtable>::value);

public:

  using input_type = t_input;
  using char_type = typename t_input::char_type;
  using size_type = typename t_input::size_type;
  using fingerprint_type = typename t_input::fingerprint_type;
  using output_type = t_output;
  using phrase_type = typename input_type::phrase_type;
  using hashtable_type = t_hashtable;

private:

  size_type m_block_size;
  input_type m_input;
  output_type m_output;
  hashtable_type m_map;

public:

  // Constructors.
  BMFast() = default;

  /// User-Friendly Constructors.
  template <typename t_char>
  BMFast(
      t_char *text,
      uint64_t length,
      uint64_t block_size = 20
  ) : m_block_size(block_size),
      m_input(text, length),
      m_output() {}

  template <typename t_char>
  BMFast(
      const std::vector<t_char>& v,
      uint64_t block_size = 20
  ) : m_block_size(block_size), 
      m_input(v),
      m_output() {}

  BMFast(
      const std::string& file_name,
      uint64_t block_size = 20
  ) : m_block_size(block_size), 
      m_input(file_name),
      m_output() {}

  BMFast(
      const std::string& input_file_name,
      const std::string& output_file_name,
      uint64_t block_size = 20
  ) : m_block_size(block_size),
      m_input(input_file_name),
      m_output(output_file_name) {}

  // Customizable Constructors.
  BMFast(
      input_type input,
      uint64_t block_size = 20
  ) : m_block_size(block_size),
      m_input(std::move(input)),
      m_output() {}

  BMFast(
      input_type input,
      output_type output,
      uint64_t block_size = 20
  ) : m_block_size(block_size),
      m_input(std::move(input)),
      m_output(std::move(output)) {}

  // Compress.
  void compress() {

    // First occurrences.
    FirstOccurrenceHashTable<char_type, fingerprint_type, size_type> first_occurrences;

    // Text index and length.
    size_type prev_text_i = 0;
    size_type text_i = 0;
    size_type text_length = m_input.size();
    size_type consecutive_literals = 0;

  fingerprint_type fingerprint{};
  if (m_block_size <= text_length) {
    // Initialize fingerprint.
    fingerprint = m_input.get_fingerprint(text_i, m_block_size);
  }

    // Process text.
    while (text_i < text_length) {

      // Best match.
      size_type best_match_start = 0;
      size_type best_match_right_extension = 1;
      size_type best_match_left_extension = 0;

      size_type bm_commit_threshold = std::max(12UL, 2 * m_block_size);
      size_type matches_to_check = std::numeric_limits<size_type>::max();

      // Search for better match.
      if (text_i + m_block_size <= text_length) {

        // Update fingerprint
        if (text_i != 0) {
          if (text_i - prev_text_i == 1) {
            char_type old_char = m_input[prev_text_i];
            char_type new_char = m_input[prev_text_i + m_block_size];
            fingerprint = m_input.get_fingerprint(fingerprint, m_block_size, old_char, new_char);
          }
          else {
            fingerprint = m_input.get_fingerprint(text_i, m_block_size);
          }
        }

        // Find matches.
        size_type match_i = 0;
        [[maybe_unused]] size_type prev_match_right_extension = m_block_size;
        auto it = m_map.find(fingerprint), end = m_map.end();
        for (; it != end; ++it, ++match_i) {

          // Initialize match.
          size_type match_start = *it;
          size_type match_right_extension = m_block_size;
          size_type match_left_extension = 0;
          size_type match_lex_smaller = true;

          // Use prefix to skip ahead.
          if constexpr(has_fingerprint_hash_table_get_prefix_length<hashtable_type>::value) {
            size_type prefix_length = m_map.get_prefix_length(it) + m_block_size;
            if (prefix_length < prev_match_right_extension) {
              prev_match_right_extension = prefix_length;
              ++match_i;
              m_map.set_match_results(it, prefix_length - m_block_size, false);
              continue;
            }
            else if (prefix_length > prev_match_right_extension) {
              ++match_i;
              m_map.set_match_results(it, prev_match_right_extension - m_block_size, false);
              continue;
            }
            else {
              match_right_extension = std::min(prefix_length, text_length - text_i);
            }
          }

          // Extend left.
          if (match_start != 0) {
            auto max_left_extension = std::min(m_block_size - 1, consecutive_literals);
            if constexpr (sizeof(char_type) == 1) {
              while (match_left_extension + 8 <= max_left_extension) {
                auto current_extend = *reinterpret_cast<const std::uint64_t*>(&m_input[text_i - match_left_extension - 8]);
                auto match_extend = *reinterpret_cast<const std::uint64_t*>(&m_input[match_start - match_left_extension - 8]);
                if (current_extend != match_extend) break;
                match_left_extension += 8;
              }
            }
            while (match_left_extension < max_left_extension) {
              auto current_extend = m_input[text_i - match_left_extension - 1];
              auto match_extend = m_input[match_start - match_left_extension - 1];
              if (current_extend != match_extend) break;
              ++match_left_extension;
            }
          }

          // Extend right 8-bytes at a time, if possible.
          if constexpr (sizeof(char_type) == 1) {
            while (text_i + match_right_extension + 8 <= text_length) {
              auto current_extend = *reinterpret_cast<const std::uint64_t*>(&m_input[text_i + match_right_extension]);
              auto match_extend = *reinterpret_cast<const std::uint64_t*>(&m_input[match_start + match_right_extension]);
              if (current_extend != match_extend) break;
              match_right_extension += 8;
            }
          }
          // Extend right.
          while (text_i + match_right_extension < text_length) {
            auto current_extend = m_input[text_i + match_right_extension];
            auto match_extend = m_input[match_start + match_right_extension];
            match_lex_smaller = current_extend > match_extend;
            if (current_extend != match_extend) break;
            ++match_right_extension;
          }

          // Tell the hashtable the length of the extension.
          if constexpr(has_fingerprint_hash_table_set_match_results<hashtable_type>::value) {
            m_map.set_match_results(it, match_right_extension - m_block_size, match_lex_smaller);
          }

          // Keep the right extension for the next match.
          prev_match_right_extension = match_right_extension;

          // Update best match.
          if (match_right_extension > best_match_right_extension) {
            best_match_start = match_start;
            best_match_right_extension = match_right_extension;
            best_match_left_extension = match_left_extension;
          }

          // Update bm commit threshold.
          if (best_match_right_extension >= bm_commit_threshold) {
            bm_commit_threshold <<=1;
            matches_to_check = match_i + 1;
          }
          else if (matches_to_check != 0) {
            --matches_to_check;
          }
          else {
            break;
          }

        }

        // Update m_map.
        if (text_i % m_block_size == 0) {
          if constexpr (has_fingerprint_hash_table_insert<hashtable_type>::value) {
            m_map.insert(fingerprint, text_i);
          }
          else {
            m_map.insert(fingerprint, text_i, it);
          }
        }

        // Handle left extension phrase removal.
        m_output.resize(m_output.size() - best_match_left_extension);

      }

      // Add phrase.
      if (best_match_right_extension == 1) {
        auto best_match_char = m_input[text_i];
        fingerprint_type char_hash = m_input.get_fingerprint(text_i, 1);
        size_type first_occurrence = first_occurrences.find(char_hash);
        if (first_occurrence == std::numeric_limits<size_type>::max()) {
          // Add a new literal.
          m_output.push_back(phrase_type(best_match_char));
          first_occurrences.insert(char_hash, text_i);
        }
        else {
          // Add a previously-seen literal.
          m_output.push_back(phrase_type(first_occurrence, 1));
        }
        ++consecutive_literals;
      }
      else {
        // Add match.
        m_output.push_back(phrase_type(best_match_start - best_match_left_extension, best_match_right_extension + best_match_left_extension));
        consecutive_literals = 0;
      }

      // Update text_i.
      prev_text_i = text_i;
      text_i = text_i + best_match_right_extension;

    }

    if constexpr (has_io_flush<output_type>::value) {
      // Flush output.
      m_output.flush();
    }

  }

  const auto& get_result() const {
    return m_output;
  }

  auto get_block_size() const {
    return m_block_size;
  }

  // Stats.
  template <typename T>
  void get_stats(T& stats) {
  }
  

};

// Deduction guides.
template <typename t_char> BMFast(t_char *text, uint64_t length) -> BMFast<InputRawString<t_char>>;
template <typename t_char> BMFast(t_char *text, uint64_t length, uint64_t block_size) -> BMFast<InputRawString<t_char>>;
template <typename t_char> BMFast(const std::vector<t_char>& v) -> BMFast<InputVector<std::vector<t_char>>>;
template <typename t_char> BMFast(const std::vector<t_char>& v, uint64_t block_size) -> BMFast<InputVector<std::vector<t_char>>>;
BMFast(const std::string& file_name) -> BMFast<InputFile<uint8_t>>;
BMFast(const std::string& file_name, uint64_t block_size) -> BMFast<InputFile<uint8_t>>;
BMFast(const std::string& input_file_name, const std::string& output_file_name) -> BMFast<InputFile<uint8_t>, OutputFile<Phrase<uint8_t, uint64_t>>>;
BMFast(const std::string& input_file_name, const std::string& output_file_name, uint64_t block_size) -> BMFast<InputFile<uint8_t>, OutputFile<Phrase<uint8_t, uint64_t>>>;

} // lz77_approx

#endif // __LZ77_APPROX__BM_FAST_HPP_INCLUDED
