cmake_minimum_required(VERSION 3.9.2...3.13)

project(stringsearch LANGUAGES CXX)

option(USE_LTO
  "Compile with -flto (link-time optimization)." ON)

include(GenerateExportHeader)

################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(USE_LTO)
  # build with link-time optimization
  include(CheckIPOSupported)
  check_ipo_supported()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(benchmark)

################################################################################
### Define source files

# glob general headers
file(GLOB_RECURSE HEADERS "include/stringsearch/*.hpp")

################################################################################
### Build

set(BUILD_SHARED_LIBS ON)

add_library(libstrsearch STATIC "src/stringsearch/Search.cpp" "src/stringsearch/SuffixSort.cpp")
target_include_directories(libstrsearch PUBLIC "span/include")
target_include_directories(libstrsearch PUBLIC "include")
target_include_directories(libstrsearch PRIVATE "src")
include(CMakePrintHelpers)
cmake_print_properties(DIRECTORIES .
  PROPERTIES
  LINK_DIRECTORIES)

add_library(strsearchdll "src/stringsearch/DllEntry.cpp")
target_include_directories(strsearchdll PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
generate_export_header(strsearchdll 
    BASE_NAME strsearchdll
    EXPORT_MACRO_NAME strsearchdll_EXPORT
    EXPORT_FILE_NAME dllexport.h
    STATIC_DEFINE strsearchdll_BUILT_AS_STATIC)
set_target_properties(strsearchdll PROPERTIES OUTPUT_NAME "strsearch")
target_link_libraries(strsearchdll libstrsearch)

add_custom_command(TARGET libstrsearch PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/testfiles $<TARGET_FILE_DIR:libstrsearch>)

add_executable(perfstrsearch "src/stringsearch/PerformanceTests.cpp")
#target_include_directories(perfstrsearch PUBLIC SYSTEM "${CMAKE_BINARY_DIR}/gbench/gbench-src/include")
target_link_libraries(perfstrsearch libstrsearch benchmark::benchmark)

link_directories(${CMAKE_BINARY_DIR})

add_executable(stringsearchx WIN32 "src/stringsearch/Main.cpp")
target_include_directories(stringsearchx PRIVATE "src")
target_include_directories(stringsearchx PUBLIC "include")
target_include_directories(stringsearchx PRIVATE "src")
target_link_libraries(stringsearchx libstrsearch)

################################################################################
