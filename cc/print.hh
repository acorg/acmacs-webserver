#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <iomanip>

// ----------------------------------------------------------------------

namespace print_internal
{
    inline void print_date_time(std::ostream& os)
    {
        const auto now = std::chrono::system_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
        const auto tt = std::chrono::system_clock::to_time_t(now);
        os << std::put_time(std::localtime(&tt), "%F %T.") << std::setw(3) << std::setfill('0') << millis;
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
        if (file) {
              // ((os<< std::this_thread::get_id() << ' ' << file << ':' << line << ": ") << ... << values) << " [" << file << ':' << line << "] " << std::this_thread::get_id() << '\n';
            (os << ... << values) << " [" << file << ':' << line << "] <" << std::this_thread::get_id() << ">\n";
        }
        else {
            os << '[';
            print_date_time(os);
            ((os << "] ") << ... << values) << " <" << std::this_thread::get_id() << ">\n";
        }
    }
}

#define print_cout(...) print_internal::print_impl(std::cout, __FILE__, __LINE__, __VA_ARGS__)
#define print_cerr(...) print_internal::print_impl(std::cerr, __FILE__, __LINE__, __VA_ARGS__)
#define print_to(to, ...) print_internal::print_impl((to), __FILE__, __LINE__, __VA_ARGS__)

inline void print_send(std::string_view&& aMessage, size_t cut_message = 100)
{
    print_internal::print_impl(std::cout, nullptr, 0, "SEND: ", aMessage.substr(0, cut_message));
}

inline void print_send_error(std::string_view&& aCommandId, std::string_view&& aMessage)
{
    print_internal::print_impl(std::cout, nullptr, 0, "ERROR: SEND: ", aCommandId, " -- ", aMessage);
}

inline void print_receive(std::string_view&& aMessage, size_t cut_message = 100)
{
    print_internal::print_impl(std::cout, nullptr, 0, "RECV: ", aMessage.substr(0, cut_message));
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
