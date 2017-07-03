#pragma once

#pragma GCC diagnostic push

// both gcc and clnag
#pragma GCC diagnostic ignored "-Wunused-parameter"

#ifdef __clang__
//#pragma GCC diagnostic ignored ""
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wshadow-uncaptured-local"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wundefined-func-template"
#endif
#include <uWS/uWS.h>
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

 // to silence clang
extern template struct uWS::Group<uWS::SERVER>;
extern template struct uWS::WebSocket<uWS::SERVER>;

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
