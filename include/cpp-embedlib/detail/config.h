// cpp-embedlib: Embed files into C++ binaries with zero dependencies.
// SPDX-License-Identifier: MIT
//
// Implementation detail. Include <cpp-embedlib/cpp-embedlib.h> instead.
#pragma once

// ---------------------------------------------------------------------------
// Language standard detection.
//
// MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is given, so use
// _MSVC_LANG (which always reflects /std:c++NN) when available.
// ---------------------------------------------------------------------------
#if defined(_MSVC_LANG)
#  define CPP_EMBEDLIB_CPLUSPLUS _MSVC_LANG
#else
#  define CPP_EMBEDLIB_CPLUSPLUS __cplusplus
#endif

#if CPP_EMBEDLIB_CPLUSPLUS < 201703L
#  error "cpp-embedlib requires C++17 or later."
#endif

// std::span is C++20. On C++17 detail/span.h provides a minimal equivalent.
// __has_include() also covers C++20 toolchains that ship an incomplete
// standard library (e.g. older AppleClang with -std=c++2a).
#if CPP_EMBEDLIB_CPLUSPLUS >= 202002L
#  if defined(__has_include)
#    if __has_include(<span>)
#      define CPP_EMBEDLIB_HAS_STD_SPAN 1
#    endif
#  else
#    define CPP_EMBEDLIB_HAS_STD_SPAN 1
#  endif
#endif

#ifndef CPP_EMBEDLIB_HAS_STD_SPAN
#  define CPP_EMBEDLIB_HAS_STD_SPAN 0
#endif
