#pragma once

// strnlen
#define _POSIX_C_SOURCE 200809L

// headers
#include "proc.hpp"     // proc_t
#include "fd.hpp"       // fd_t

// posix
#include <string.h>     // strnlen
#include <unistd.h>     // getuid
#include <sys/types.h>  // getuid, getpwuid
#include <pwd.h>        //         getpwuid

// c
#include <cstdio>       // printf
#include <cstring>      // strncpy, strncat

// cpp
#include <map>          // map
#include <filesystem>   // fs::*
#include <utility>      // move
#include <algorithm>    // count
#include <optional>     // optional
#include <stdexcept>    // runtime_error


constexpr int MAX_CLIENTS = 5;


inline const auto CONF = std::filesystem::path{ ".apps.json" };
inline const auto SOCK = std::filesystem::path{ ".socket" };

inline auto CONF_PATH = std::filesystem::path{};
inline auto SOCK_PATH = std::filesystem::path{};
inline auto LOG_PATH  = std::filesystem::path{};


inline void setup_paths(bool log = false)
{
    namespace fs = std::filesystem;

    struct passwd* pw = getpwuid(getuid());
    if (!pw)
        throw std::runtime_error("cannot obtain HOME");

    auto home = fs::path{ pw->pw_dir };

    auto dir = home / ".srvctl";
    CONF_PATH = dir / CONF;
    SOCK_PATH = dir / SOCK;
    LOG_PATH  = dir;

    if (!fs::is_regular_file(CONF_PATH))
        throw std::runtime_error("'" + CONF_PATH.string()
                                 += "' doesn't exist or isn't a regular file");

    if (log)
        std::fprintf(stderr, "CONF_PATH: %s\n", CONF_PATH.c_str());
}


struct argv_t
{
    std::string data;
    std::vector<char*> ptrs;

    char* const* get() const { return ptrs.data(); }

    argv_t(std::string str) : data(std::move(str))
    {
        ptrs.reserve(2 + std::count(data.begin(), data.end(), ' '));

        ptrs.push_back(&data[0]);

        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i] == ' ')
            {
                data[i] = '\0';
                ptrs.push_back(&data[i + 1]);   // +1 doesn't cause problems,
            }                                   // because std::string will
        }                                       // contain terminating null
        ptrs.push_back(nullptr);
    }
};


struct app_t
{
    std::filesystem::path dir;
    argv_t start;
    argv_t update;
    std::optional<decltype(std::declval<proc_t>().wait())> exit{};
};


struct server_t
{
    std::map<std::string, proc_t> procs;
    std::map<std::string, app_t> apps;
    fd_t sock{ -1 };

    auto reap(decltype(procs)::iterator it)
    {
        auto& proc = it->second;
        auto ex = proc.wait();
        apps.at(it->first).exit = ex;
        return procs.erase(it);
    }

    void reap_zombies()
    {
        for (auto it = procs.begin(); it != procs.end(); )
        {
            auto& proc = it->second;
            if (proc.zombie())
                it = reap(it);
            else
                ++it;
        }
    }
};
