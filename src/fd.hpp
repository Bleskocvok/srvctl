#pragma once

// posix
#include <unistd.h>     // read, write, close

// cpp
#include <utility>      // exchange


struct fd_t
{
    int val = -1;

    fd_t(int file_desc)
        : val(file_desc)
    { }

    ~fd_t()
    {
        if (val != -1)
            ::close(val);
    }

    fd_t(const fd_t&) = delete;
    fd_t& operator=(const fd_t&) = delete;

    fd_t(fd_t&& other) noexcept
        : val(std::exchange(other.val, -1))
    { }

    fd_t& operator=(fd_t&& other) noexcept
    {
        val = std::exchange(other.val, -1);
        return *this;
    }

    explicit operator bool() const { return val != -1; }

    ssize_t read(char* buf, size_t count)
    {
        return ::read(val, buf, count);
    }

    ssize_t write(const char* buf, size_t count)
    {
        return ::write(val, buf, count);
    }
};
