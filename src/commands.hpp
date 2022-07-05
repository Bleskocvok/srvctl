#pragma once


#include "common.hpp"   // app_t, message_t, server_t
#include "proc.hpp"     // proc


#include <map>      // map


using cmd_ptr = message (*) (const message&, server_t&);


struct command
{
    cmd_ptr func;
    std::string desc;
};


extern const std::map<std::string, command> commands;
