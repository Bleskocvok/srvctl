#pragma once

// posix
#include <unistd.h>     // read, write, close

// cpp
#include <utility>      // exchange


struct fd_t
{
    int fd = -1;

    fd_t(int file_desc)
        : fd(file_desc)
    { }

    ~fd_t()
    {
        if (fd != -1)
            ::close(fd);
    }

    fd_t(const fd_t&) = delete;
    fd_t& operator=(const fd_t&) = delete;

    fd_t(fd_t&& other) noexcept
        : fd(std::exchange(other.fd, -1))
    { }

    fd_t& operator=(fd_t&& other) noexcept
    {
        fd = std::exchange(other.fd, -1);
        return *this;
    }

    explicit operator bool() const { return fd != -1; }

    ssize_t read(char* buf, size_t count)
    {
        return ::read(fd, buf, count);
    }

    ssize_t write(const char* buf, size_t count)
    {
        return ::write(fd, buf, count);
    }
};
