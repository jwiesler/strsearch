cmake_minimum_required(VERSION 3.9.2...3.13)

project(strsearch LANGUAGES CXX)

option(STRSEARCH_ENABLE_SHARED "Build a shared library" ON)
option(STRSEARCH_ENABLE_TESTS "Compile tests" OFF)
option(STRSEARCH_ENABLE_BENCHMARK "Compile benchmark" OFF)
option(STRSEARCH_ENABLE_MAIN "Compile main" OFF)

option(STRSEARCH_USE_LTO "Compile with -flto (link-time optimization)." ON)

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(STRSEARCH_USE_LTO)
  # build with link-time optimization
  include(CheckIPOSupported)
  check_ipo_supported()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

################################################################################
### Build

add_library(libstrsearch STATIC "src/stringsearch/Search.cpp" "src/stringsearch/SuffixSort.cpp")
target_include_directories(libstrsearch PUBLIC "include" "span/include")
target_include_directories(libstrsearch PRIVATE "src")

add_custom_command(TARGET libstrsearch PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/testfiles $<TARGET_FILE_DIR:libstrsearch>)

if(STRSEARCH_ENABLE_SHARED)
	message("Building shared library.")

	add_library(strsearchdll SHARED "src/stringsearch/DllEntry.cpp")

	target_include_directories(strsearchdll PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	generate_export_header(strsearchdll 
		BASE_NAME strsearchdll
		EXPORT_MACRO_NAME strsearchdll_EXPORT
		EXPORT_FILE_NAME dllexport.h
		STATIC_DEFINE strsearchdll_BUILT_AS_STATIC)

	target_compile_definitions(strsearchdll PRIVATE -fPIC)
	set_target_properties(strsearchdll PROPERTIES 
		PREFIX ""
		OUTPUT_NAME "strsearch")
	target_link_libraries(strsearchdll libstrsearch)
endif()

if(STRSEARCH_ENABLE_BENCHMARK)
	message("Building benchmarks.")

	option(BM_RADIX_SORT "Benchmark radix sort" ON)
	option(BM_SUFFIX_ARRAY_FIND "Benchmark suffix array find" ON)
	option(BM_UNIQUE "Find unique results" ON)
	
	set(BENCHMARK_ENABLE_TESTING OFF)
	add_subdirectory(benchmark)
	add_executable(perfstrsearch "src/stringsearch/PerformanceTests.cpp")
	
	if(BM_RADIX_SORT)
		target_compile_definitions(perfstrsearch PUBLIC BM_RADIX_SORT)
	endif()

	if(BM_SUFFIX_ARRAY_FIND)
		target_compile_definitions(perfstrsearch PUBLIC BM_SUFFIX_ARRAY_FIND)
	endif()

	if(BM_UNIQUE)
		target_compile_definitions(perfstrsearch PUBLIC BM_UNIQUE)
	endif()
	target_link_libraries(perfstrsearch libstrsearch benchmark::benchmark)
endif()

if(STRSEARCH_ENABLE_TESTS)
	message("Building tests.")

	add_executable(teststringsearch "src/stringsearch/Test.cpp")
	target_include_directories(teststringsearch PRIVATE "Catch2/single_include")
	target_link_libraries(teststringsearch libstrsearch)
endif()

################################################################################
