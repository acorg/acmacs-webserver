#pragma once

#pragma GCC diagnostic push

#include "acmacs-base/boost-diagnostics.hh"

// both gcc and clnag
#pragma GCC diagnostic ignored "-Wunused-parameter"

#ifdef __clang__
//#pragma GCC diagnostic ignored ""
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
