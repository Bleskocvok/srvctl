#pragma once

// headers
#include "fd.hpp"       // fd_t

// c
#include <cstdio>       // snprintf
#include <cstring>      // strncpy, strncat
#include <cerrno>       // errno

// cpp
#include <array>        // array
#include <vector>       // vector
#include <optional>     // optional


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
