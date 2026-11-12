#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>

#include "bm_fast.hpp"

void show_help(std::string executable_name) {
  std::cerr << std::left;
  std::cerr << "usage: " << executable_name << " input_file [output_file] [-h] [-b BLOCK_SIZE] [-d] [-v]" << std::endl;
  std::cerr << std::setw(16) << "  output_file "   << "File to write compressed or decompressed output." << std::endl;
  std::cerr << std::setw(16) << " "                << "Default during compression: input_file.approx_lz77." << std::endl;
  std::cerr << std::setw(16) << " "                << "Default during decompression: input_file.decomp." << std::endl;
  std::cerr << std::setw(16) << "  -h "            << "Show help." << std::endl;
  std::cerr << std::setw(16) << "  -b BLOCK_SIZE " << "Block size for Bentley-McIlroy algorithm" << std::endl;
  std::cerr << std::setw(16) << "  -d "            << "Decompress input to output." << std::endl;
  std::cerr << std::setw(16) << "  -v "            << "Show verbose output." << std::endl;
}

void compress(
  std::string input_fpath,
  std::string output_fpath,
  uint64_t block_size,
  bool verbose
) {
  // Define tracking variables.
  double time_sec = 0;
  uint64_t phrase_count = 0;

  {
    // Compress and write to file.
    lz77_approx::BMFast index(input_fpath, output_fpath, block_size);
    auto timer_start = std::chrono::high_resolution_clock::now();
    index.compress();
    auto timer_end = std::chrono::high_resolution_clock::now();

    // Update tracking variables.
    time_sec = std::chrono::duration<double>(timer_end - timer_start).count();
    phrase_count = index.get_result().size();
  }

  // // Or equivalently:
  // {
  //   // Compress.
  //   lz77_approx::BMFast index(input_fpath, block_size);
  //   auto timer_start = std::chrono::high_resolution_clock::now();
  //   index.compress();
  //   auto timer_end = std::chrono::high_resolution_clock::now();
  // 
  //   // Write to file
  //   std::ofstream ofs(output_fpath, std::ios::binary);
  //   const auto& phrases = index.get_result(); 
  //   for (uint64_t phrase_i = 0; phrase_i < phrases.size(); ++phrase_i) {
  //     uint64_t length = phrases[phrase_i].length;
  //     if (length == 0) {
  //       uint64_t chr = phrases[phrase_i].chr;
  //       ofs.write(reinterpret_cast<char *>(&chr), sizeof(uint64_t));
  //     }
  //     else {
  //       uint64_t start = phrases[phrase_i].start;
  //       ofs.write(reinterpret_cast<char *>(&start), sizeof(uint64_t));
  //     }
  //     ofs.write(reinterpret_cast<char *>(&length), sizeof(uint64_t));
  //   }
  //   ofs.flush();

  //   // Update tracking variables.
  //   time_sec = std::chrono::duration<double>(timer_end - timer_start).count();
  //   phrase_count = index.get_result().size();
  // }

  // Verbose.
  if (verbose) {
    std::cout << "Finished compressing " << input_fpath;
    std::cout << " (size = " << lz77_approx::get_file_size_bytes(input_fpath) << " B)";
    std::cout << " to " << output_fpath;
    std::cout << " (size = " << lz77_approx::get_file_size_bytes(output_fpath) << " B)";
    std::cout << std::endl;
    std::cout << "  Time (sec) = " << time_sec << std::endl;
    std::cout << "  Phrase Count = " << phrase_count << std::endl;
  }
}


int main(int argc, char *argv[])
{
  std::string input_fpath = "";
  std::string output_fpath = "";
  uint64_t block_size = 20;
  bool verbose = false;
  bool do_decompress = false;

  // Ensure at least one argument is passed.
  if (argc <= 1) {
    std::cerr << "Error: missing input file" << std::endl;
    show_help(argv[0]);
    std::exit(EXIT_FAILURE);
  }

  // Handle -h case.
  if (std::string(argv[1]) == "-h") {
    show_help(argv[0]);
    std::exit(EXIT_SUCCESS);
  }

  // Parse input file.
  input_fpath = std::string(argv[1]);

  // Parse output file and options.
  int arg_i = 2;
  while (arg_i < argc) {
    std::string arg(argv[arg_i]);
    if (arg == "-h") {
      show_help(argv[0]);
      std::exit(EXIT_SUCCESS);
    }
    else if (arg == "-b") {
      block_size = std::stoull(argv[++arg_i]);
    }
    else if (arg == "-d") {
      do_decompress = true;
    }
    else if (arg == "-v") {
      verbose = true;
    }
    else {
      output_fpath = arg;
    }
    ++arg_i;
  }

  // Default output file.
  if (output_fpath.empty()) {
    if (do_decompress) {
      output_fpath = input_fpath + ".decomp";
    }
    else {
      output_fpath = input_fpath + ".approx_lz77";
    }
  }

  if (do_decompress) {
    lz77_approx::decompress_lz_parse(input_fpath, output_fpath, verbose);
  }
  else {
    compress(input_fpath, output_fpath, block_size, verbose);
  }

}
