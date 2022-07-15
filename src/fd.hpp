#pragma once

// posix
#include <unistd.h>     // read, write, close, dup, dup2
#include <stdio.h>      // fileno

// cpp
#include <cstdio>       // FILE
#include <utility>      // exchange


struct fd_t
{
    int fd = -1;

    static int fileno(std::FILE* file) { return ::fileno(file); }

    fd_t(int file_desc)
        : fd(file_desc)
    { }

    ~fd_t()
    {
        if (fd != -1)
            close();
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

    int close() { return ::close(std::exchange(fd, -1)); }

    void reset() { fd = -1; }

    fd_t dup() const
    {
        return fd_t{ ::dup(fd) };
    }

    fd_t dup2(int newfd) const
    {
        return fd_t{ ::dup2(fd, newfd) };
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
