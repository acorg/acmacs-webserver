#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <thread>

// ----------------------------------------------------------------------

namespace print_internal
{
    inline void print_prefix(std::ostream& os, const char* file, int line)
    {
        os << std::this_thread::get_id() << ' ' << file << ':' << line << ": ";
    }

    template <typename Value> inline void print_value(std::ostream& os, Value value) { os << value; }

    inline void print_values(std::ostream&) {}

    template <typename Value, typename ... Values> inline void print_values(std::ostream& os, Value value, Values ... values)
    {
        print_value(os, value);
        print_values(os, values ...);
    }

    template <typename ... Values> inline void print_impl(std::ostream& os, const char* file, int line, Values ... values)
    {
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
        static std::mutex access;
#pragma GCC diagnostic pop
        std::unique_lock<decltype(access)> lock{access};
        print_prefix(os, file, line);
        print_values(os, values ...);
        os << std::endl;
    }
}

#define print_cout(...) print_internal::print_impl(std::cout, __FILE__, __LINE__, __VA_ARGS__)
#define print_cerr(...) print_internal::print_impl(std::cerr, __FILE__, __LINE__, __VA_ARGS__)

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
