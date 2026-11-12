#ifndef __LZ77_APPROX__UTILS_HPP_INCLUDED
#define __LZ77_APPROX__UTILS_HPP_INCLUDED

#include <cassert>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <iostream>
#include <fstream>
#include <random>
#include <unordered_map>
#include <vector>

namespace lz77_approx {

// has_reset_peak_ram.
template<typename T, typename = void> struct has_reset_peak_ram : std::false_type {};
template<typename T> struct has_reset_peak_ram<T, std::void_t<decltype(T::reset_peak_ram())>> : std::true_type {}; 

// has_get_peak_ram.
template<typename T, typename = void> struct has_get_peak_ram : std::false_type {};
template<typename T> struct has_get_peak_ram<T, std::void_t<decltype(T::get_peak_ram())>> : std::true_type {}; 

// has_get_stats.
template<typename T, typename U, typename = void> struct has_get_stats : std::false_type {};
template<typename T, typename U> struct has_get_stats<T, U, std::void_t<decltype(std::declval<T>().get_stats(std::declval<U>()))>> : std::true_type {}; 

// has_serialize
template<typename T, typename = void> struct has_serialize : std::false_type {};
template<typename T> struct has_serialize<T, std::enable_if_t<std::is_integral_v<decltype(std::declval<T>().serialize(std::declval<std::ostream&>()))>>> : std::true_type {}; 

// has_load
template<typename T, typename = void> struct has_load : std::false_type {};
template<typename T> struct has_load<T, std::void_t<decltype(std::declval<T>().load(std::declval<std::istream&>()))>> : std::true_type {}; 

// Null buffer.
class NullBuffer : public std::streambuf {
public:
  int overflow(int c) override { return c; }
};

// Null output stream.
class NullOutputStream : public std::ostream {
private:
  NullBuffer buf;
public:
  NullOutputStream() : std::ostream(&buf) {}
}; 

// MSb
inline uint64_t most_significant_bit(uint64_t x) {
  return (x == 0) ? 0 : (63 - __builtin_clzll(x));
}

// Generate a random number in range [low, high].
std::uint64_t random_number(std::uint64_t low, std::uint64_t high, std::mt19937_64& generator) {
  std::uniform_int_distribution<std::uint64_t> distribution(low, high);
  return distribution(generator);
}

// For mersenne primes, k = 2, 3, 5, 7, 13, 17, 19, 31, 61
constexpr std::uint64_t get_mersenne_number(std::uint64_t k) {
  return (1ULL << k) - 1;
}

// Return a * b mod n.
std::uint64_t get_a_by_b_mod_n(std::uint64_t a, std::uint64_t b, std::uint64_t n) {
  return (static_cast<unsigned __int128>(a) * b) % n;
}

// Return a^k mod n. Calculate naively.
std::uint64_t get_a_to_k_mod_n(std::uint64_t a, std::uint64_t k, std::uint64_t n) {
  unsigned __int128 result = 1 % n;
  for (std::uint64_t i = 0; i < k; ++i) {
    result *= a;
    result %= n;
  }
  return result;
}

std::uint64_t get_a_by_b_mod_mersenne(const std::uint64_t a, const std::uint64_t b, const std::uint64_t k) {
  const std::uint64_t p = ((std::uint64_t)1 << k) - 1;
  const unsigned __int128 ab = static_cast<unsigned __int128>(a) * b;
  std::uint64_t lo = (std::uint64_t)ab;
  const std::uint64_t hi = (ab >> 64);
  lo = (lo & p) + ((lo >> k) + (hi << (64 - k)));
  lo = (lo & p) + (lo >> k);
  return lo == p ? 0 : lo;
}

std::uint64_t get_a_mod_mersenne(std::uint64_t a, const std::uint64_t k) {
  const std::uint64_t p = get_mersenne_number(k);
  if (k < 32) {

    // We need to check if a <= 2^(2k).
    const std::uint64_t threshold = ((std::uint64_t)1 << (k << 1));
    if (a <= threshold) {
      a = (a & p) + (a >> k);
      a = (a & p) + (a >> k);
      return a == p ? 0 : a;
    } else return a % p;
  } else {

    // We are guaranteed that a < 2^(2k)
    // because a < 2^64 <= 2^(2k).
    a = (a & p) + (a >> k);
    a = (a & p) + (a >> k);
    return a == p ? 0 : a;
  }
}

std::uint64_t get_a_to_n_mod_mersenne(const std::uint64_t a, std::uint64_t n, const std::uint64_t k) {
  std::uint64_t pow = get_a_mod_mersenne(a, k);
  std::uint64_t ret = get_a_mod_mersenne(1, k);
  while (n > 0) {
    if (n & 1)
      ret = get_a_by_b_mod_mersenne(ret, pow, k);
    pow = get_a_by_b_mod_mersenne(pow, pow, k);
    n >>= 1;
  }
  return ret;
}

uint64_t get_file_size_bytes(const std::string& text_name) {
  uint64_t file_size_bytes = 0;
  {
    std::ifstream is(text_name, std::ios::binary | std::ios::ate);
    assert(is.good());
    file_size_bytes = is.tellg();
    is.close();
  }
  return file_size_bytes;
}

// Decompress LZ-like parse from input_fpath to output_fpath.
// Each phrase is a pair of 64-bit unsigned integers.
// The second value in a phrase is the length of the phrase or zero.
// The first value in a phrase is a character if the second value is zero.
// Otherwise, it is the starting position of a phrase references.
void decompress_lz_parse(
  std::string input_fpath,
  std::string output_fpath,
  bool verbose
) {
  // Open file.
  std::ifstream ifs(input_fpath, std::ios::binary | std::ios::ate);
  if (!ifs.good()) {
    std::cerr << "Error: input file could not be opened";
    std::cerr << " (input_fpath = " << input_fpath << ")." << std::endl;
  }

  // Verify file size is valid.
  ifs.seekg(0, std::ios::end); 
  uint64_t input_file_size = ifs.tellg();
  if ((input_file_size & 15) != 0) {
    std::cerr << "Error: input file is not a LZ77 parse.";
    std::cerr << " File size is not a multiple of 16";
    std::cerr <<  "(input_file_size = " << input_file_size << ").";
    std::exit(EXIT_FAILURE);
  }
  ifs.seekg(0, std::ios::beg);

  // Get parse.
  uint64_t n_phrases = input_file_size >> 4;
  using phrase_type = std::pair<uint64_t, uint64_t>;
  std::vector<phrase_type> phrases;
  phrases.reserve(n_phrases);
  for (uint64_t phrase_i = 0; phrase_i < n_phrases; ++phrase_i) {
    phrase_type phrase;
    ifs.read(reinterpret_cast<char*>(&(phrase.first)), 8);
    ifs.read(reinterpret_cast<char*>(&(phrase.second)), 8);
    phrases.push_back(phrase);
  }

  // Ensure only the first occurrence of symbol uses phrase with length 0.
  std::unordered_map<uint64_t, uint64_t> symbol_to_first_occurrence;
  for (std::uint64_t phrase_i = 0, text_i = 0; phrase_i < phrases.size(); ++phrase_i) {
    uint64_t length = phrases[phrase_i].second;
    if (length == 0) {
      uint64_t chr = phrases[phrase_i].first;
      auto first_occ_p = symbol_to_first_occurrence.find(chr);
      if (first_occ_p != symbol_to_first_occurrence.end()) {
        std::cerr << "Error: The phrase for the symbol " << chr;
        std::cerr << " at " << text_i;
        std::cerr << " does not refer to the first occurrence of that symbol";
        std::cerr << " at " << first_occ_p->second;
        std::cerr << std::endl;
        std::exit(EXIT_FAILURE);
      }
      else {
        symbol_to_first_occurrence.insert(std::make_pair(chr, text_i));
      }
      ++text_i;
    }
    text_i += length;
  }

  // Get text length.
  std::uint64_t output_file_size = 0;
  for (std::uint64_t phrase_i = 0; phrase_i < phrases.size(); ++phrase_i) {
    output_file_size += std::max<uint64_t>(phrases[phrase_i].second, 1UL);
  }

  // Decompress text.
  std::string result;
  result.resize(output_file_size);
  for (std::uint64_t phrase_i = 0, text_i = 0; phrase_i < phrases.size(); ++phrase_i) {
    auto phrase = phrases[phrase_i];
    if (phrase.second == 0) {
      result[text_i] = phrase.first;
      ++text_i;
    }
    else {
      for (uint64_t i = 0; i < phrase.second; ++i) {
        result[text_i + i] = result[phrase.first + i];
      }
      text_i += phrase.second;
    }
  }

  // Open output file.
  std::ofstream ofs(output_fpath, std::ios::binary);
  if (!ofs.good()) {
    std::cerr << "Error: output file could not be opened";
    std::cerr << "(output_fpath = " << output_fpath << ")." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Write text to output file.
  ofs.write(result.c_str(), result.size());
  ofs.close();
}

template <typename t_index, typename t_phrases>
void write_phrases_to_file(const std::string& output_fpath, const t_phrases& phrases) {
    using index_type = t_index;

    // Get first occurrences. Assumes byte-sized characters.
    std::array<uint64_t, 256> first_occurrences;
    first_occurrences.fill(std::numeric_limits<uint64_t>::max());

    // Write parsing to file.
    std::ofstream ofs(output_fpath, std::ios::binary);
    for (uint64_t phrase_i = 0, text_i = 0; phrase_i < phrases.size(); ++phrase_i) {
      uint64_t length = index_type::phrase_length(phrases[phrase_i]);
      if (length == 0) {
        uint64_t chr = index_type::phrase_char(phrases[phrase_i]);
        // Write the phrase for a single character.
        // Ensure only the first occurrence uses phrases with length 0.
        if (first_occurrences[chr] == std::numeric_limits<uint64_t>::max()) {
          // Write the phrase for the first occurrence of a character.
          ofs.write(reinterpret_cast<char *>(&chr), sizeof(uint64_t));
          ofs.write(reinterpret_cast<char *>(&length), sizeof(uint64_t));
          first_occurrences[chr] = text_i;
        }
        else {
          // Write the phrase for a character that appeared previously.
          uint64_t start = first_occurrences[chr];
          length = 1;
          ofs.write(reinterpret_cast<char *>(&start), sizeof(uint64_t));
          ofs.write(reinterpret_cast<char *>(&length), sizeof(uint64_t));
        }
        ++text_i;
      }
      else {
        // Write the phrase for a single character.
        uint64_t start = index_type::phrase_start(phrases[phrase_i]);
        ofs.write(reinterpret_cast<char *>(&start), sizeof(uint64_t));
        ofs.write(reinterpret_cast<char *>(&length), sizeof(uint64_t));
        text_i += length;
      }
    }
    ofs.flush();

}

} // lz77_approx

#endif // __LZ77_APPROX__UTILS_HPP_INCLUDED
