
#include "common.hpp"   // message

#include <unistd.h>     // write, read, close
#include <sys/socket.h> // bind, socket, connect, listen, accept
#include <sys/types.h>  // bind, socket, connect, listen, accept
#include <sys/un.h>     // sockaddr_un

#include <cstdio>       // printf, perror
#include <cstring>      // strncpy

#include <fstream>      // ifstream
#include <iostream>     // cout
#include <map>          // map
#include <filesystem>   // fs::*
#include <algorithm>    // min


namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    message msg{};

    if (argc <= 1)
        return std::fprintf(stderr, "usage: %s CMD [ARG]\n", argv[0]), 1;

    std::strncpy(msg.cmd, argv[1], sizeof(msg.cmd));
    std::strncpy(msg.arg, argv[2], sizeof(msg.arg));

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        std::perror("(srvctl) ERROR");
        return 1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH_STR, std::min(sizeof(addr.sun_path),
                                                        sizeof(SOCK_PATH_STR)));

    if (connect(sock_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        std::perror("(srvctl) ERROR");
        return 1;
    }

    write(sock_fd, reinterpret_cast<char*>(&msg), msg.size());

    read(sock_fd, reinterpret_cast<char*>(&msg), msg.size());

    std::cout << "response:\n"
              << "'" << msg.str_cmd() << "'"
              << "\n"
              << "'" << msg.str_arg() << "'"
              << std::endl;

    close(sock_fd);

    return 0;
}

