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
#include <cstdio>       // snprintf

// cpp
#include <map>          // map
#include <filesystem>   // fs::*
#include <utility>      // move
#include <algorithm>    // transform, count
#include <optional>     // optional
#include <array>        // array


constexpr int MAX_CLIENTS = 5;


const auto CONF_PATH = std::filesystem::path{ ".srvctl.json" };
const auto SOCK_PATH = std::filesystem::path{ ".srvctl.sock" };


template<size_t Size>
std::string to_str(const char* str)
{
    return { str, ::strnlen(str, Size) };
}


struct message
{
    static constexpr size_t Block = 256;

    char arg[Block] = { 0 };
    std::vector<std::array<char, Block>> contents = {};

    message() = default;

    template<typename ... Args>
    message(const char* arg_, const char* fmt, Args&& ... args)
    {
        set_arg(arg_);
        add_line(fmt, std::forward<Args>(args)...);
    }

    message(const char* arg_, const char* nxt = nullptr)
    {
        set_arg(arg_);
        if (nxt)
        {
            contents.emplace_back(std::array<char, Block>{ 0 });
            std::strncpy(line(0), nxt, Block);
        }
    }

    void set_arg(const char* arg_) { std::strncpy(arg, arg_, Block); }

    const char* line(size_t i) const { return contents[i].data(); }
          char* line(size_t i)       { return contents[i].data(); }

    auto send(fd_t& out) -> std::optional<int>
    {
        unsigned char size = contents.size();
        if (out.write(reinterpret_cast<char*>(&size), 1) == -1)
            return { errno };

        if (out.write(reinterpret_cast<char*>(&arg), sizeof(arg) - 1) == -1)
            return { errno };

        for (size_t i = 0; i < size; ++i)
        {
            if (out.write(line(i), Block - 1) == -1)
                return { errno };
        }

        return {};
    }

    auto recv(fd_t& in) -> std::optional<int>
    {
        contents.clear();

        unsigned char size = 0;
        if (in.read(reinterpret_cast<char*>(&size), 1) == -1)
            return { errno };

        auto r = in.read(reinterpret_cast<char*>(&arg), sizeof(arg) - 1);
        if (r == -1)
            return { errno };

        arg[r] = '\0';

        for (size_t i = 0; i < size; ++i)
        {
            contents.emplace_back(std::array<char, Block>{ 0 });
            if (in.read(contents.back().data(), Block - 1) == -1)
                return { errno };
        }

        return {};
    }

    template<typename ... Args>
    int add_line(const char* fmt, Args&& ... args)
    {
        contents.emplace_back(std::array<char, Block>{ 0 });
        return std::snprintf(contents.back().data(), Block, fmt,
                             std::forward<Args>(args)...);
    }

    int add_line(const char* str)
    {
        contents.emplace_back(std::array<char, Block>{ 0 });
        std::strncat(contents.back().data(), str, Block);
        // TODO: use strnlen
        return Block;
    }
};


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
