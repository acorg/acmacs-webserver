#pragma once

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#include "acmacs-base/boost-diagnostics.hh"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
// for websocketpp::frame::opcode, websocketpp::config::asio::message_type
#include "websocketpp/config/asio.hpp"
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
