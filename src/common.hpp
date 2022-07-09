#pragma once

// strnlen
#define _POSIX_C_SOURCE 200809L

// headers
#include "proc.hpp"     // proc_t
#include "fd.hpp"       // fd_t

// posix
#include <string.h>     // strnlen

// cpp
#include <map>          // map
#include <filesystem>   // fs::*
#include <utility>      // move
#include <algorithm>    // transform, count
#include <optional>     // optional


constexpr int MAX_CLIENTS = 5;


const auto CONF_PATH = std::filesystem::path{ ".srvctl.json" };
const auto SOCK_PATH = std::filesystem::path{ ".srvctl.sock" };


template<size_t Size>
std::string to_str(const char* str)
{
    return { str, ::strnlen(str, Size) };
}


template<size_t CmdSize, size_t ArgSize>
struct message_t
{
    static constexpr size_t block = 256;

    static constexpr size_t size() { return CmdSize + ArgSize; }

    char cmd[CmdSize] = { 0 };
    char arg[ArgSize] = { 0 };

    auto str_cmd() const { return to_str<sizeof(cmd)>(cmd); }
    auto str_arg() const { return to_str<sizeof(arg)>(arg); }

    auto ser(fd_t& out) -> std::optional<int>
    {
        return {};
    }

    auto deser(fd_t& in) -> std::optional<int>
    {
        char arg[64] = { 0 };
        auto res = std::vector<char[block]>{};

        char size = 0;
        if (in.read(&size, 1) == -1)
            return { errno };

        if (in.read(&arg, sizeof(arg) - 1) == -1)
            return { errno };

        for (size_t i = 0; i < size; ++i)
        {
            res.push_back({ 0 });
            if (in.read(&res.back(), block - 1) == -1)
                return { errno };
        }

        return {};
    }
};


using message = message_t<64, 1024>;


struct argv_t
{
    std::string data;
    std::vector<char*> ptrs;

    char* const* c_str() const { return ptrs.data(); }

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

    void reap_zombies() { reap_zombies([](const auto&, const auto&){}); }

    template<typename ForEachZombie>
    void reap_zombies(ForEachZombie func)
    {
        for (auto it = procs.begin(); it != procs.end(); )
        {
            auto& proc = it->second;
            if (proc.zombie())
            {
                auto ex = proc.wait();
                func(ex, it);
                apps.at(it->first).exit = ex;

                it = procs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
};
