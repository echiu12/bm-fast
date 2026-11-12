BM-Fast: Fast LZ77 Approximation
===

Description
---

`bm-fast` ([include/bm_fast.hpp](include/bm_fast.hpp)) is an
implementation of a fast LZ77 approximation algorithm
for highly repetitive texts. It is based on the Bentley&ndash;McIlroy compression algorithm [1].

Usage
---

Build:

    make

Usage:

    build/lz77_approx input_file [output_file] [-h] [-b BLOCK_SIZE] [-d] [-v]
      output_file   File to write compressed or decompressed output.
                    Default during compression: input_file.approx_lz77
                    Default during decompression: input_file.decomp
      -h            Show help.
      -b BLOCK_SIZE Block size for text to approximate lz77 (Bentley-McIlroy algorithm)
      -d            Decompress input to output.
      -v            Show verbose output.

Examples:

    # Compress file `TEXT` to file `TEXT.approx_lz77`
    build/lz77_approx TEXT

    # Decompress file `TEXT.approx_lz77` to file `TEXT.approx_lz77.decomp`
    build/lz77_approx TEXT.approx_lz77 -d

References
---
[1] Jon Louis Bentley, M. Douglas McIlroy: Data Compression Using Long Common Strings. Data Compression Conference 1999: 287-295
