#pragma once

// headers
#include "message.hpp"  // message
#include "common.hpp"   // app_t, server_t
#include "proc.hpp"     // proc

// cpp
#include <map>          // map
#include <vector>       // vector


using cmd_ptr = message (*) (const message&, server_t&);


struct command
{
    cmd_ptr func;
    std::vector<std::string> usage;
    std::vector<std::string> desc;
};


extern const std::map<std::string, command> COMMANDS;


void print_help(const char* program);

