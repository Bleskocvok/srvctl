
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
#include "message.hpp"  // message
#include "common.hpp"   // app, to_str
#include "log.hpp"      // *log*
#include "fd.hpp"       // fd

// deps
#include "deps/json.hpp"

// posix
#include <fcntl.h>      // fcntl
#include <unistd.h>     // fcntl, daemon
#include <sys/socket.h> // bind, socket, connect, listen, accept
#include <sys/types.h>  // bind, socket, connect, listen, accept
#include <sys/un.h>     // sockaddr_un
#include <poll.h>       // ppoll
#include <signal.h>     // ppoll, sig_atomic_t, sigaction, sigemptyset,
                        //        sigaddset, sigprocmask, sigsuspend, SIG*

// c
#include <cstdio>       // printf
#include <cstring>      // strncpy, memset
#include <cerrno>       // errno

// cpp
#include <fstream>      // ifstream
#include <map>          // map
#include <filesystem>   // fs::*
#include <array>        // array


using json = nlohmann::json;


namespace fs = std::filesystem;


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


void print_usage(const char* argv0)
{
    std::printf("usage: %s [--no-daemon | -nod]]\n", argv0);
}


int run(int argc, char** argv)
{
    setup_paths(true);

    using namespace std::literals;

    bool deamonize = true;
    if (argc > 1)
    {
        if (argv[1] == "--no-daemon"sv || argv[1] == "-nod"sv)
            deamonize = false;
        else if (argv[1] == "--help"sv)
            return print_usage(argv[0]), 0;
        else
            return print_usage(argv[0]), 1;
    }

    if (deamonize)
    {
        if (daemon(0, 0) == -1)
            return log_errno(errno), 1;
        log_output(syslog_tag{});
    }

    setup_react_signals();

    sigset_t mask_set,
             mask_old;
    sigemptyset(&mask_set);

    for (size_t i = 0; i < react_sigs.size(); i++)
        sigaddset(&mask_set, react_sigs[i]);

    if (sigprocmask(SIG_BLOCK, &mask_set, &mask_old) == -1)
        return log_errno(errno), 1;

    server_t server;
    server.apps = parse(CONF_PATH);

    server.sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!server.sock)
        return log_errno(errno), 1;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH.c_str(), sizeof(addr.sun_path));

    fs::remove(SOCK_PATH);

    if (bind(server.sock.fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
        return log_errno(errno), 1;

    if (listen(server.sock.fd, MAX_CLIENTS) == -1)
        return log_errno(errno), 1;

    // // TODO: perhaps:
    // prctl(PR_SET_CHILD_SUBREAPER, ...);

    struct pollfd fds = { server.sock.fd, POLLIN | POLLOUT, 0 };

    struct timespec delay;
    delay.tv_sec = 3;
    delay.tv_nsec = 0;

    message msg;

    fcntl(server.sock.fd, F_SETFL, O_NONBLOCK);

    while (true)
    {
        if (ppoll(&fds, 1, &delay, &mask_old) == 0)
            continue;

        if (reactions.terminate)
            return 0;

        if (reactions.child_dead == 1)
        {
            reactions.child_dead = 0;
            server.reap_zombies();
        }

        fd_t client = accept4(server.sock.fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (!client)
        {
            int err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK)
                continue;

            log_errno(errno);
            continue;  // do not exit in case a client connection fails
        }

        msg.recv(client);

        auto it = COMMANDS.find(msg.arg);
        if (it != COMMANDS.end())
        {
            auto& cmd = it->second;
            auto resp = cmd.func(msg, server);
            resp.send(client);
        }
        else
        {
            auto resp = message{ "invalid cmd", msg.arg };
            resp.send(client);
            log_err("invalid cmd '", msg.arg, "'");
        }
    }

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
        log_err(e.what());
        return 1;
    }
}
