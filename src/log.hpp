#pragma once

// posix
#include <syslog.h>     // syslog, openlog

// c
#include <cstdio>       // printf
#include <cstring>      // strerror

// cpp
#include <utility>      // forward, declval
#include <type_traits>  // is_same_v, decay_t


inline static bool USE_SYSLOG = false;


struct syslog_tag {};
struct stderr_tag {};

void log_output(syslog_tag)
{
    ::openlog("srvd", LOG_PID, LOG_USER);
    USE_SYSLOG = true;
}


void log_output(stderr_tag)
{
    USE_SYSLOG = false;
}


template<typename ... Args>
void logf(const char* fmt, Args&& ... args)
{
    if (USE_SYSLOG)
        ::syslog(LOG_ERR, fmt, std::forward<Args>(args)...);
    else
        std::fprintf(stderr, fmt, std::forward<Args>(args)...);
}


template<typename Arg, typename ... Args>
void log_err_rec(Arg&& arg, Args&& ... args)
{
    using T = std::decay_t<Arg>;
    if constexpr (std::is_same_v<char,
                                 std::decay_t<decltype(std::declval<T>()[0])>>)
    {
        logf("%s", arg);
    }
    else if constexpr (std::is_same_v<std::string, T>)
    {
        logf("%s", arg.c_str());
    }
    else
    {
        using D = typename std::decay<decltype(std::declval<T>()[0])>::type;
        std::declval<D>().get_type_bla_bla();
    }

    if constexpr (sizeof...(Args) != 0)
        log_err_rec(std::forward<Args>(args)...);
}


template<typename ... Args>
void log_err(Args&& ... args)
{
    log_err_rec("(srvd) ERROR: ");
    log_err_rec(std::forward<Args>(args)...);
    log_err_rec("\n");
}


inline void log_errno(int err) { log_err(std::strerror(err)); }
