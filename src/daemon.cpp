
// daemon
#define _DEFAULT_SOURCE
// sigaction, siginfo_t
#define _POSIX_C_SOURCE 200809L
// ppoll
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include <poll.h>       // ppoll
#include <signal.h>     // ppoll, sig_atomic_t, sigaction, sigemptyset,
                        //        sigaddset, sigprocmask, sigsuspend, SIG*

// c
#include <cstdio>       // printf, perror
#include <cstring>      // strncpy, strerror, memset

// cpp
#include <fstream>      // ifstream
#include <iostream>     // cout
#include <map>          // map
#include <filesystem>   // fs::*
#include <algorithm>    // min
#include <array>        // array


using json = nlohmann::json;


namespace fs = std::filesystem;


static server_t server;


struct reactions_t
{
    sig_atomic_t terminate{ 0 };
    sig_atomic_t child_dead{ 0 };

} static volatile reactions;


static const std::array react_sigs =
{
    SIGCHLD,    // important, our need to reap child
    SIGTERM,    // term
    SIGINT,     // term
    SIGALRM,    // term
    SIGABRT,    // core
    SIGQUIT,    // core
};


void handler(int sig,
             [[maybe_unused]] siginfo_t* info,
             [[maybe_unused]] void* ucontext)
{
    switch (sig)
    {
        case SIGCHLD: reactions.child_dead = 1; break;

        // term
        case SIGTERM: [[fallthrough]];
        case SIGINT:  [[fallthrough]];
        case SIGALRM: reactions.terminate = 1; break;

        // core
        case SIGABRT: [[fallthrough]];
        case SIGQUIT: reactions.terminate = 1; break;

        default: break;
    }
}


void setup_react_signals()
{
    for (size_t i = 0; i < react_sigs.size(); i++)
    {
        struct sigaction act;
        std::memset(&act, 0, sizeof(act));
        act.sa_sigaction = handler;
        act.sa_flags = SA_SIGINFO,
        sigaction(react_sigs[i], &act, nullptr);
    }
}


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

    setup_react_signals();

    sigset_t mask_set,
             mask_old;
    sigemptyset(&mask_set);

    for (size_t i = 0; i < react_sigs.size(); i++)
        sigaddset(&mask_set, react_sigs[i]);

    if (sigprocmask(SIG_BLOCK, &mask_set, &mask_old) == -1)
        return std::perror("(srvd) ERROR"), 1;

    server.apps = parse(CONF_PATH);

    fd_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!sock)
        return std::perror("(srvd) ERROR"), 1;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH.c_str(), sizeof(addr.sun_path));

    fs::remove(SOCK_PATH);

    if (bind(sock.fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        std::perror("(srvd) ERROR");
        return 1;
    }

    if (listen(sock.fd, MAX_CLIENTS) == -1)
    {
        std::perror("(srvd) ERROR");
        return 1;
    }

    // // TODO: perhaps:
    // prctl(PR_SET_CHILD_SUBREAPER, ...);

    struct pollfd fds = { sock.fd, POLLIN | POLLOUT, 0 };

    struct timespec delay;
    delay.tv_sec = 3;
    delay.tv_nsec = 0;

    message msg;

    fcntl(sock.fd, F_SETFL, O_NONBLOCK);

    while (true)
    {
        if (ppoll(&fds, 1, &delay, &mask_old) == 0)
            continue;

        if (reactions.terminate)
            return 0;

        if (reactions.child_dead == 1)
        {
            reactions.child_dead = 0;
            std::printf("SIGCHLD\n");
            // TODO: reap the zombies
            for(auto it  = server.procs.begin();
                     it != server.procs.end();)
            {
                auto& proc = it->second;
                if (proc.zombie())
                {
                    auto ex = proc.wait();
                    switch (ex.index())
                    {
                        case 0: std::printf("chld exit %d\n",
                                            std::get<exitted>(ex).ret);
                                break;
                        case 1: std::printf("chld sig  %d\n",
                                            std::get<signalled>(ex).sig);
                                break;
                    }
                    std::printf("'%s' waited\n", it->first.c_str());
                    it = server.procs.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        fd_t client = accept4(sock.fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (!client)
        {
            int err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK)
                continue;

            std::fprintf(stderr, "(srvd) ERROR: %s\n", std::strerror(err));
            return 1;
        }

        client.read(reinterpret_cast<char*>(&msg), msg.size());

        auto it = commands.find(msg.cmd);
        if (it != commands.end())
        {
            auto& cmd = it->second;
            auto resp = cmd.func(msg, server);
            client.write(reinterpret_cast<char*>(&resp), resp.size());
        }
        else
        {
            std::cerr << "invalid cmd '"
                        << msg.str_cmd()
                        << "'"
                        << std::endl;
        }
    }

    return 0;
}

