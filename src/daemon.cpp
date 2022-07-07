
// daemon
#define _DEFAULT_SOURCE

// headers
#include "commands.hpp" // commands
#include "common.hpp"   // message, app, to_str
#include "proc.hpp"     // proc
#include "fd.hpp"       // fd

// deps
#include "deps/json.hpp"

// posix
#include <fcntl.h>      // fcntl
#include <unistd.h>     // fcntl, daemon, write, read, close
#include <sys/socket.h> // bind, socket, connect, listen, accept
#include <sys/types.h>  // bind, socket, connect, listen, accept
#include <sys/un.h>     // sockaddr_un
#include <errno.h>      // errno

// c
#include <cstdio>       // printf, perror
#include <cstring>      // strncpy, strerror

// cpp
#include <fstream>      // ifstream
#include <iostream>     // cout
#include <map>          // map
#include <filesystem>   // fs::*
#include <algorithm>    // min


using json = nlohmann::json;


namespace fs = std::filesystem;


static server_t server;


std::map<std::string, app_t> parse(const fs::path& path)
{
    auto data = json{};
    auto in = std::ifstream(path);
    in >> data;
    in.close();

    auto result = std::map<std::string, app_t>{};

    for (auto& [key, value] : data.items())
    {
        result.emplace(key, app_t{ value["dir"], argv_t{ value["start"]  },
                                                 argv_t{ value["update"] } });
    }
    return result;
}


int main(int argc, char** argv)
{
    // if (daemon(0, 0) == -1)
    // {
    //     std::perror("(srvctl deamon) ERROR");
    //     return 1;
    // }

    // deamon now

    server.apps = parse(CONF_PATH);

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        std::perror("(srvd) ERROR");
        return 1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH_STR, std::min(sizeof(addr.sun_path),
                                                        sizeof(SOCK_PATH_STR)));

    fs::remove(SOCK_PATH);

    if (bind(sock_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        std::perror("(srvd) ERROR");
        return 1;
    }

    if (listen(sock_fd, MAX_CLIENTS) == -1)
    {
        std::perror("(srvd) ERROR");
        return 1;
    }

    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    message msg;

    while (true)
    {
        static int i = 0;
        printf("%d\n", i++);
        int client = accept4(sock_fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (client == -1)
        {
            int err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK)
                continue;

            std::fprintf(stderr, "(srvd) ERROR: %s\n", std::strerror(err));
            return 1;
        }

        read(client, reinterpret_cast<char*>(&msg), msg.size());

        auto it = commands.find(msg.cmd);
        if (it != commands.end())
        {
            auto& cmd = it->second;
            auto resp = cmd.func(msg, server);
            write(client, reinterpret_cast<char*>(&resp), resp.size());
        }
        else
        {
            std::cerr << "invalid cmd '"
                        << msg.str_cmd()
                        << "'"
                        << std::endl;
        }

        close(client);
    }

    close(sock_fd);

    return 0;
}

