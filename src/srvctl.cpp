
// headers
#include "message.hpp"  // message
#include "common.hpp"   // *_PATH
#include "fd.hpp"       // fd_t

// posix
#include <unistd.h>     // write, read, close
#include <sys/socket.h> // bind, socket, connect, listen, accept
#include <sys/types.h>  // bind, socket, connect, listen, accept
#include <sys/un.h>     // sockaddr_un

// c
#include <cstdio>       // printf, perror
#include <cstring>      // strncpy, strerror
#include <cerrno>       // errno

// cpp
#include <fstream>      // ifstream
#include <iostream>     // cout
#include <map>          // map
#include <filesystem>   // fs::*
#include <string_view>  // string_view


namespace fs = std::filesystem;


void log_err(int err)
{
    std::fprintf(stderr, "(srvctl) ERROR: %s\n", std::strerror(err));
}


int main(int argc, char** argv)
{
    setup_paths();

    if (argc <= 1)
        return std::fprintf(stderr, "usage: %s CMD [ARG]\n", argv[0]), 1;

    auto msg = message{ argv[1], argc > 2 ? argv[2] : nullptr};

    fd_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!sock)
        return log_err(errno), 1;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH.c_str(), sizeof(addr.sun_path));

    if (connect(sock.fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        return log_err(errno), 1;

    if (auto err = msg.send(sock))
        return log_err(*err), 1;

    if (auto err = msg.recv(sock))
        return log_err(*err), 1;

    std::cout << "[" << msg.arg << "]\n";
    for (const auto& line : msg.contents)
        std::cout << line.data() << std::endl;

    using namespace std::literals;

    if (msg.arg == "error"sv)
        return 1;

    return 0;
}

