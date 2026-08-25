cmake_minimum_required(VERSION 3.16)

# ---- version from the VERSION file ----------------------------------------
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _MKSLUG_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${_MKSLUG_VERSION_RAW}" MKSLUG_VERSION)

project(mkslug VERSION ${MKSLUG_VERSION} LANGUAGES C
        DESCRIPTION "Fast slugify/unslugify CLI")

set(MKSLUG_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(MKSLUG_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(MKSLUG_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# ---- standards / defaults -------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

# ---- locate keg-only Homebrew deps on macOS -------------------------------
if(APPLE)
  find_program(HOMEBREW_EXECUTABLE brew)
  if(HOMEBREW_EXECUTABLE)
    execute_process(COMMAND ${HOMEBREW_EXECUTABLE} --prefix icu4c@78
                    OUTPUT_VARIABLE _icu_prefix OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(NOT _icu_prefix)
      execute_process(COMMAND ${HOMEBREW_EXECUTABLE} --prefix icu4c
                      OUTPUT_VARIABLE _icu_prefix
                      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    if(_icu_prefix)
      set(ICU_ROOT "${_icu_prefix}" CACHE PATH "ICU root")
      list(APPEND CMAKE_PREFIX_PATH "${_icu_prefix}")
    endif()
    execute_process(COMMAND ${HOMEBREW_EXECUTABLE} --prefix pcre2
                    OUTPUT_VARIABLE _pcre2_prefix
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(_pcre2_prefix)
      list(APPEND CMAKE_PREFIX_PATH "${_pcre2_prefix}")
    endif()
  endif()
endif()

# ---- dependencies ---------------------------------------------------------
find_package(ICU REQUIRED COMPONENTS uc i18n data)

set(_PCRE2_TARGET "")
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PCRE2 IMPORTED_TARGET libpcre2-8)
  if(PCRE2_FOUND)
    set(_PCRE2_TARGET PkgConfig::PCRE2)
  endif()
endif()
if(NOT _PCRE2_TARGET)
  find_path(PCRE2_INCLUDE_DIR pcre2.h)
  find_library(PCRE2_LIBRARY NAMES pcre2-8 libpcre2-8 pcre2-8-static)
  if(NOT PCRE2_LIBRARY OR NOT PCRE2_INCLUDE_DIR)
    message(FATAL_ERROR "PCRE2 (libpcre2-8) not found; install pcre2 / libpcre2-dev")
  endif()
  add_library(pcre2_imported INTERFACE)
  target_include_directories(pcre2_imported INTERFACE ${PCRE2_INCLUDE_DIR})
  target_link_libraries(pcre2_imported INTERFACE ${PCRE2_LIBRARY})
  set(_PCRE2_TARGET pcre2_imported)
endif()

# ---- generated version header (parity with autotools) ---------------------
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mkslug/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/gen/mkslug/version.h @ONLY)

# ---- sources --------------------------------------------------------------
set(MKSLUG_LIB_SOURCES
  src/options.c
  src/config.c
  src/slug.c
  src/clipboard.c
  src/pattern.c
  src/fileops.c
  src/util.c)

add_executable(mkslug src/main.c ${MKSLUG_LIB_SOURCES})
target_include_directories(mkslug PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_compile_definitions(mkslug PRIVATE MKSLUG_VERSION="${MKSLUG_VERSION}")
target_link_libraries(mkslug PRIVATE ICU::uc ICU::i18n ICU::data ${_PCRE2_TARGET})

if(WIN32)
  target_link_libraries(mkslug PRIVATE user32)
endif()

if(MSVC)
  target_compile_options(mkslug PRIVATE /W3 /utf-8)
else()
  target_compile_options(mkslug PRIVATE -Wall -Wextra)
endif()

# ---- install --------------------------------------------------------------
install(TARGETS mkslug RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES man/mkslug.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# ---- tests ----------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  foreach(_t test_slug test_pattern)
    add_executable(${_t} test/${_t}.c ${MKSLUG_LIB_SOURCES})
    target_include_directories(${_t} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_BINARY_DIR}/gen)
    target_compile_definitions(${_t} PRIVATE MKSLUG_VERSION="${MKSLUG_VERSION}")
    target_link_libraries(${_t} PRIVATE ICU::uc ICU::i18n ICU::data ${_PCRE2_TARGET})
    if(WIN32)
      target_link_libraries(${_t} PRIVATE user32)
    endif()
    add_test(NAME ${_t} COMMAND ${_t})
  endforeach()

  if(NOT WIN32)
    add_test(NAME cli_test
             COMMAND ${CMAKE_COMMAND} -E env MKSLUG=$<TARGET_FILE:mkslug>
                     sh ${CMAKE_CURRENT_SOURCE_DIR}/test/cli_test.sh)
  endif()
endif()
