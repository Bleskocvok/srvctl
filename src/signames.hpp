#pragma once

// cpp
#include <utility>      // pair


extern const std::pair<const char*, int> SIGNAMES[];


const char* str_sig(int);

int int_sig(const char*);
