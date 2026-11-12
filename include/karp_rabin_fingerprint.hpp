#ifndef __LZ77_APPROX__KARP_RABIN_FINGERPRINT_HPP_INCLUDED
#define __LZ77_APPROX__KARP_RABIN_FINGERPRINT_HPP_INCLUDED

#include <cstdint>
#include <random>
#include <type_traits>

#include "utils.hpp"

namespace lz77_approx {

class FingerprintManager {
public:

  using fingerprint_type = uint64_t;

  static inline uint64_t k = 61;
  static inline uint64_t q = get_mersenne_number(k);
  static inline uint64_t r = 488747828876043682ULL;

  static void init(uint64_t new_k = 61) {
    k = new_k;
    q = get_mersenne_number(k);
    r = rand() % q + 2;
  }

  template <typename T>
  static fingerprint_type hash_char(T c) {
    std::cerr << "Error: hash_char of char type not defined." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  static fingerprint_type hash_char(uint8_t c) { return c; }
  static fingerprint_type hash_char(char c) { return c; }

  template <typename t_iterator>
  static fingerprint_type get_fingerprint(t_iterator begin, t_iterator end) {
    uint64_t h = 0;
    while (begin != end) {
      h = get_a_by_b_mod_mersenne(h, r, k);
      h = get_a_mod_mersenne(h + hash_char(*begin) + 1, k);
      ++begin;
    }
    return h;
  }

  template <typename t_char>
  static fingerprint_type get_fingerprint(fingerprint_type fingerprint, uint64_t length, t_char old_char, t_char new_char) {
    fingerprint = unconcat(hash_char(old_char) + 1, fingerprint, length - 1);
    fingerprint = concat(fingerprint, hash_char(new_char) + 1, 1);
    return fingerprint;
  }

  static fingerprint_type concat(fingerprint_type left_hash, fingerprint_type right_hash, uint64_t right_len) {
    fingerprint_type pow = get_a_to_n_mod_mersenne(r, right_len, k);
    fingerprint_type tmp = get_a_by_b_mod_mersenne(left_hash, pow, k);
    fingerprint_type ret = get_a_mod_mersenne(tmp + right_hash, k);
    return ret;
  }

  static fingerprint_type unconcat(fingerprint_type left_hash, fingerprint_type combined_hash, uint64_t right_len) {
    fingerprint_type pow = get_a_to_n_mod_mersenne(r, right_len, k);
    fingerprint_type tmp = get_a_by_b_mod_mersenne(left_hash, pow, k);
    fingerprint_type mersenne_prime = get_mersenne_number(k);
    fingerprint_type ret = get_a_mod_mersenne((combined_hash + mersenne_prime) - tmp, k);
    return ret;
  }

};

} // namespace lz77_approx

#endif // __LZ77_APPROX__KARP_RABIN_FINGERPRINT_HPP_INCLUDED
