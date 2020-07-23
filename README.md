# strsearch #

strsearch is a C++ implementation of an infix search on UTF-16-LE strings using suffix and other lookup arrays.
It has the following features:
* Radixsort implementations (in place, own buffer and shared buffer for the reordering step after filling the buckets)
* Lookup of an infix in `O(log n)`
* Finding entries of unique items (separated by `\0` in the original string) in a suffix array range in `O(r)` where `r` is the size of the range. This works by looking up the suffix array location of the last entry of the same item.

## Installation ##
Using the CMake script. The following build options are availible:
* `STRSEARCH_ENABLE_BENCHMARK` builds the benchmarks (currently requires Windows)
* `STRSEARCH_ENABLE_TESTS` builds the tests
* `STRSEARCH_ENABLE_SHARED` builds a shared library (default on)