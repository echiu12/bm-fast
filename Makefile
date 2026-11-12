# DIRECTORIES
build_dir      := build
include_dir    := include
src_dir        := src

# CONFIG
CC       := gcc
CXX      := g++
CFLAGS  := -Wall -O3 -funroll-loops -march=native
CXXFLAGS := $(CFLAGS) -std=c++17
CPPFLAGS := -DNDEBUG

#######
# ALL #
#######

.PHONY: all
all: $(build_dir)/lz77_approx

###############
# LZ77_APPROX #
###############

$(build_dir)/lz77_approx: override CPPFLAGS += -Iinclude
$(build_dir)/lz77_approx: $(src_dir)/lz77_approx.cpp $(wildcard include/**)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

#########
# CLEAN #
#########

.PHONY: clean
clean:
	rm -rf $(build_dir)
