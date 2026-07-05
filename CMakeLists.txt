cmake_minimum_required(VERSION 3.25)

if(APPLE)
  message(FATAL_ERROR "macOS is not supported by this Node addon build.")
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
  set(_HTMLKIT_VCPKG_TOOLCHAIN
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  if(EXISTS "${_HTMLKIT_VCPKG_TOOLCHAIN}")
    set(CMAKE_TOOLCHAIN_FILE "${_HTMLKIT_VCPKG_TOOLCHAIN}" CACHE STRING "")
  endif()
endif()

project(htmlkit LANGUAGES C CXX)

if(WIN32 AND DEFINED VCPKG_TARGET_TRIPLET
    AND NOT VCPKG_TARGET_TRIPLET MATCHES "windows-static$")
  message(FATAL_ERROR "Windows builds must use a *-windows-static vcpkg triplet.")
endif()

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
if(MSVC)
  set(_HTMLKIT_MSVC_RUNTIME_LIBRARY
      "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_MSVC_RUNTIME_LIBRARY
      "${_HTMLKIT_MSVC_RUNTIME_LIBRARY}" CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_ARGN "--static" CACHE STRING "Arguments for pkg-config" FORCE)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pangocairo pango)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

find_package(Fontconfig REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(GIF REQUIRED)
find_package(JPEG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(libavif CONFIG REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
find_package(Threads REQUIRED)

if(TARGET unofficial::gumbo::gumbo AND NOT TARGET gumbo::gumbo)
  add_library(gumbo::gumbo ALIAS unofficial::gumbo::gumbo)
endif()
if(TARGET unofficial::gumbo::gumbo AND NOT TARGET gumbo)
  add_library(gumbo ALIAS unofficial::gumbo::gumbo)
endif()

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LITEHTML_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EXTERNAL_GUMBO ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  litehtml
  GIT_REPOSITORY https://github.com/BlueGlassBlock/litehtml.git
  GIT_TAG 9e1eced0ac5936fce28a7d3692868196afcfe083
)
FetchContent_MakeAvailable(litehtml)

if(NOT TARGET litehtml)
  message(FATAL_ERROR "Fetched litehtml target not found")
endif()

if(TARGET avif)
  set(HTMLKIT_AVIF_TARGET avif)
elseif(TARGET avif::avif)
  set(HTMLKIT_AVIF_TARGET avif::avif)
elseif(TARGET libavif::avif)
  set(HTMLKIT_AVIF_TARGET libavif::avif)
else()
  message(FATAL_ERROR "libavif CMake target not found")
endif()

find_program(NODE_EXECUTABLE node REQUIRED)
execute_process(
  COMMAND "${NODE_EXECUTABLE}" -p "require('node-addon-api').include_dir"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)

add_library(htmlkit SHARED
  src/addon.cpp
  core/base64.cpp
  core/cairo_wrapper.cpp
  core/debug_container.cpp
  core/font_wrapper.cpp
  core/htmlkit_container.cpp
  core/render_engine.cpp
  core/resource_provider.cpp
)

set_target_properties(htmlkit PROPERTIES
  PREFIX ""
  SUFFIX ".node"
)

target_include_directories(htmlkit PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/core"
  "${NODE_ADDON_API_DIR}"
  ${CMAKE_JS_INC}
)

target_compile_definitions(htmlkit PRIVATE
  NAPI_CPP_EXCEPTIONS
  NAPI_VERSION=8
  UNICODE
)

if(MSVC)
  set_property(TARGET htmlkit PROPERTY MSVC_RUNTIME_LIBRARY
    "${_HTMLKIT_MSVC_RUNTIME_LIBRARY}")
  if(TARGET litehtml)
    set_property(TARGET litehtml PROPERTY MSVC_RUNTIME_LIBRARY
      "${_HTMLKIT_MSVC_RUNTIME_LIBRARY}")
  endif()
  target_compile_options(htmlkit PRIVATE /EHsc /utf-8 /bigobj)
endif()

target_link_libraries(htmlkit PRIVATE
  ${CMAKE_JS_LIB}
  litehtml
  unofficial::gumbo::gumbo
  ${HTMLKIT_AVIF_TARGET}
  PkgConfig::PANGO
  PkgConfig::CAIRO
  Fontconfig::Fontconfig
  fmt::fmt
  GIF::GIF
  JPEG::JPEG
  WebP::webp
  WebP::webpdecoder
  WebP::webpdemux
  Threads::Threads
)

if(WIN32)
  target_link_libraries(htmlkit PRIVATE Dwrite)
endif()

if(UNIX AND NOT APPLE)
  target_link_options(htmlkit PRIVATE -static-libstdc++ -static-libgcc)
  target_link_libraries(htmlkit PRIVATE
    -static-libstdc++
    -static-libgcc
    -Wl,-Bstatic
    stdc++
    gcc
    gcc_eh
    -Wl,-Bdynamic
  )
endif()
