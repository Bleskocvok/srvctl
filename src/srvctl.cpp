
// headers
#include "commands.hpp" // COMMANDS, print_help
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
#include <string_view>  // ""sv


namespace fs = std::filesystem;


void log_err(int err)
{
    std::fprintf(stderr, "ERROR: %s\n", std::strerror(err));
}


int run(int argc, char** argv)
{
    setup_paths();

    using namespace std::literals;

    if (argc <= 1)
        return std::fprintf(stderr, "Usage: %s CMD [ARG]\n", argv[0]), 1;

    if (argv[1] == "--help"sv || argv[1] == "help"sv)
        return print_help(argv[0]), 0;

    if (COMMANDS.count(argv[1]) == 0)
    {
        std::fprintf(stderr, "Invalid command.\nUsage: %s CMD [ARG]\n"
                             "\n"
                             "Try: %s help\n", argv[0], argv[0]);
        return 1;
    }

    auto msg = message{ argv[1] };

    for (int i = 2; i < argc; i++)
        msg.add_line(argv[i]);

    fd_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!sock)
        return log_err(errno), 1;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH.c_str(), sizeof(addr.sun_path));

    if (connect(sock.fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        return log_err(errno),
               printf("\nPerhaps the daemon is not running,\n"
                      "try starting it with ‹srvd› first\n"), 1;

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


int main(int argc, char** argv)
{
    try
    {
        return run(argc, argv);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr, "ERROR: %s\n", e.what());
        return 1;
    }
}

