#pragma once

// strnlen
#define _POSIX_C_SOURCE 200809L

// headers
#include "proc.hpp"     // proc

// posix
#include <string.h>     // strnlen

// cpp
#include <map>          // map
#include <filesystem>   // fs::*
#include <utility>      // move
#include <algorithm>    // transform, count


const auto CONF_PATH = std::filesystem::path{ ".srvctl.json" };

const char SOCK_PATH_STR[] = ".srvctl.sock";
const auto SOCK_PATH = std::filesystem::path{ SOCK_PATH_STR };

constexpr int MAX_CLIENTS = 5;


template<size_t Size>
std::string to_str(const char* str)
{
    return { str, ::strnlen(str, Size) };
}


template<size_t CmdSize, size_t ArgSize>
struct message_t
{
    static constexpr size_t size() { return CmdSize + ArgSize; }

    char cmd[CmdSize] = { 0 };
    char arg[ArgSize] = { 0 };

    auto str_cmd() const { return to_str<sizeof(cmd)>(cmd); }
    auto str_arg() const { return to_str<sizeof(arg)>(arg); }
};


using message = message_t<64, 1024>;


struct argv_t
{
    std::string data;
    std::vector<char*> ptrs;

    argv_t(std::string str)
        : data(std::move(str))
    {
        data.reserve(1 + std::count(data.begin(), data.end(), ' '));

        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i] == ' ')
            {
                data[i] = '\0';
                // +1 doesn't cause problems, because std::string will
                // contain terminating null
                ptrs.push_back(&data[i + 1]);
            }
        }
        ptrs.push_back(nullptr);
    }
};


struct app_t
{
    std::filesystem::path dir;
    argv_t start;
    argv_t update;
};


struct server_t
{
    std::map<std::string, proc> procs;
    std::map<std::string, app_t> apps;
};
