
#include "common.hpp"   // message
#include "fd.hpp"       // fd_t

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
    if (argc <= 1)
        return std::fprintf(stderr, "usage: %s CMD [ARG]\n", argv[0]), 1;

    // std::strncpy(msg.arg, argv[1], sizeof(msg.arg));

    // if (argc > 2)
    //     std::strncpy(msg.arg, argv[2], sizeof(msg.arg));

    auto msg = message{ argv[1], argc > 2 ? argv[2] : nullptr};

    fd_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!sock)
        return std::perror("(srvctl) ERROR"), 1;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH.c_str(), sizeof(addr.sun_path));

    if (connect(sock.fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        return std::perror("(srvctl) ERROR"), 1;

    msg.send(sock);

    msg.recv(sock);

    // sock.write(reinterpret_cast<char*>(&msg), msg.size());

    // sock.read(reinterpret_cast<char*>(&msg), msg.size());

    std::cout << "response: '" << msg.arg << "'\n";

    for (const auto& line : msg.contents)
    {
        std::cout << line.data() << std::endl;
    }

    return 0;
}

