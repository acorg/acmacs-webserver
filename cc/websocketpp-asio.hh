#pragma once

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wtautological-type-limit-compare"
#pragma GCC diagnostic ignored "-Wtautological-unsigned-enum-zero-compare"

// websocketpp has "using std::auto_ptr" not available in C++17
// simulate it
namespace std { template <typename T> using auto_ptr = unique_ptr<T>; }

#endif
// for websocketpp::frame::opcode, websocketpp::config::asio::message_type
#include "websocketpp/config/asio.hpp"
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
