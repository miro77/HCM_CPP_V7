cmake_minimum_required(VERSION 3.21)
project(hcmms
  VERSION 7.21.5
  DESCRIPTION "C++17 port of HCMMS V7 (Harmonized Calculation Method for mobile services)"
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(HCMMS_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(HCMMS_IS_TOP_LEVEL ON)
endif()

option(HCMMS_BUILD_CLI   "Build the hcmms command line tool" ${HCMMS_IS_TOP_LEVEL})
option(HCMMS_BUILD_TESTS "Build the tests"                   ${HCMMS_IS_TOP_LEVEL})

# Numerical fidelity (docs/adr/0002): no FMA contraction, evaluation in 53-bit
# double (also on 32-bit x86, where the x87 unit would otherwise use 64 bits).
function(hcmms_set_compile_options target)
  target_compile_features(${target} PRIVATE cxx_std_17)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /fp:precise /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR MATCHES "i.86|x86|AMD64")
      target_compile_options(${target} PRIVATE -msse2 -mfpmath=sse)
    endif()
  endif()
endfunction()

# --- Library -----------------------------------------------------------------
add_library(hcmms STATIC
  src/antenna.cpp
  src/cases.cpp
  src/figure_tables.cpp
  src/figures.cpp
  src/files.cpp
  src/fortran_io.cpp
  src/geo.cpp
  src/hcm_ms.cpp
  src/line.cpp
  src/mobile.cpp
  src/p2p.cpp
  src/permissible.cpp
  src/terrain.cpp
)
add_library(hcmms::hcmms ALIAS hcmms)
target_compile_features(hcmms PUBLIC cxx_std_17)
target_include_directories(hcmms
  PUBLIC  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE ${PROJECT_SOURCE_DIR}/src)
hcmms_set_compile_options(hcmms)

# --- Command line tool ----------------------------------------------------------
if(HCMMS_BUILD_CLI)
  add_executable(hcmms_cli app/main.cpp)
  set_target_properties(hcmms_cli PROPERTIES OUTPUT_NAME hcmms)
  # The CLI uses the case-file runner, an internal component of the library.
  target_include_directories(hcmms_cli PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(hcmms_cli PRIVATE hcmms::hcmms)
  hcmms_set_compile_options(hcmms_cli)
endif()

# --- Tests -----------------------------------------------------------------------
if(HCMMS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# --- Install -----------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS hcmms EXPORT hcmmsTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/hcmms DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT hcmmsTargets NAMESPACE hcmms:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hcmms
  FILE hcmmsConfig.cmake)
if(HCMMS_BUILD_CLI)
  install(TARGETS hcmms_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
