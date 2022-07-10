#pragma once

// strnlen
#define _POSIX_C_SOURCE 200809L

// headers
#include "proc.hpp"     // proc_t
#include "fd.hpp"       // fd_t

// posix
#include <string.h>     // strnlen

// c
#include <cstring>      // strncpy, strncat

// cpp
#include <map>          // map
#include <filesystem>   // fs::*
#include <utility>      // move
#include <algorithm>    // count
#include <optional>     // optional


constexpr int MAX_CLIENTS = 5;


const auto CONF_PATH = std::filesystem::path{ ".srvctl.json" };
const auto SOCK_PATH = std::filesystem::path{ ".srvctl.sock" };


struct argv_t
{
    std::string data;
    std::vector<char*> ptrs;

    char* const* get() const { return ptrs.data(); }

    argv_t(std::string str) : data(std::move(str))
    {
        data.reserve(1 + std::count(data.begin(), data.end(), ' '));

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
