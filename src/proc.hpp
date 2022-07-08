#pragma once

// kill
#define _POSIX_C_SOURCE 200809L

// posix
#include <sys/wait.h>       // kill, waitpid
#include <sys/types.h>      //       waitpid, fork
#include <unistd.h>         //                fork, exec*
#include <signal.h>         // kill
#include <sys/prctl.h>      // prctl

// c
#include <cstdlib>          // exit

// cpp
#include <stdexcept>        // runtime_error
#include <variant>          // variant
#include <vector>           // vector
#include <filesystem>       // fs::*
#include <utility>          // exchange


struct exitted   { int ret; };
struct signalled { int sig; };


struct proc
{
    pid_t pid = -1;

    proc(char* const argv[], const std::filesystem::path& cwd)
    {
        pid = ::fork();

        if (pid == -1)
            throw std::runtime_error("fork");

        if (pid == 0)
        {
            if (::prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
                // TODO: better log
                std::perror("(srvd) ERROR");

            std::filesystem::current_path(cwd);

            ::execvp(argv[0], argv);

            // TODO: error log
            std::exit(1);
        }
    }

    proc(const proc&) = delete;
    proc& operator=(const proc&) = delete;

    proc(proc&& other) noexcept
        : pid(std::exchange(other.pid, -1))
    { }

    proc& operator=(proc&& other) noexcept
    {
        pid = std::exchange(other.pid, -1);
        return *this;
    }

    ~proc()
    {
        if (pid == -1)
            return;

        proc::signal(SIGKILL);
        proc::wait();
    }

    bool zombie() const
    {
        return pid != -1 && !running();
    }

    bool running() const
    {
        if (pid == -1)
            return false;

        siginfo_t info = { 0, 0, 0, 0, 0 };
        ::waitid(P_PID, pid, &info, WEXITED | WNOHANG | WNOWAIT);

        return info.si_pid != pid;
    }

    auto wait() -> std::variant<exitted, signalled>
    {
        if (pid == -1)
            throw std::runtime_error("proc::wait");

        int status{};
        pid_t r = ::waitpid(pid, &status, 0);

        if (r == -1)
            throw std::runtime_error("waitpid");

        pid = -1;

        if (WIFEXITED(status))
            return exitted{ WEXITSTATUS(status) };

        if (WIFSIGNALED(status))
            return signalled{ WTERMSIG(status) };

        throw std::runtime_error("proc::wait: invalid state");
    }

    void signal(int sig)
    {
        if (pid == -1)
            return;

        if (::kill(pid, sig) == -1)
            throw std::runtime_error("kill");
    }
};
