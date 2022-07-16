#include "commands.hpp"

// headers
#include "fd.hpp"       // fd_t
#include "signames.hpp" // str_sig

// cpp
#include <string>       // string
#include <variant>      // get_if
#include <array>        // array


namespace fs = std::filesystem;


message cmd_start (const message&, server_t&);
message cmd_stop  (const message&, server_t&);
message cmd_update(const message&, server_t&);
message cmd_list  (const message&, server_t&);
message cmd_signal(const message&, server_t&);


extern const std::map<std::string, command> COMMANDS =
{
    { "start",  command{ cmd_start,
                         { "APP" },
                         { "Start app by given name.",
                           "This app name must be present in ",
                           "the respective configuration file." } } },
    { "stop",   command{ cmd_stop,
                         { "APP" },
                         { "Stop a running instance of app of the given name.",
                           "It must be running.",
                           "The app is stopped by sending SIGKILL." } } },
    { "update", command{ cmd_update,
                         { "APP" },
                         { "Update a given app. If the app is currently running,",
                           "it is first stopped as if by command ‹stop›." } } },
    { "list",   command{ cmd_list,
                         {},
                         { "List each apps loaded from the configuration file",
                           "If an instance is running, PID is listed.",
                           "If the app had been stopped, information about",
                           "signal/return is listed." } } },
    { "signal", command{ cmd_signal,
                         { "APP", "SIGNAL"},
                         { "" } } },
    // TODO:
    // { "status", command{ cmd_status, {}, {} } },
    // { "signal", command{ cmd_status, {}, {} } },
    // { "log",    command{ cmd_status, {}, {} } },
};


void print_help(const char* program)
{
    std::printf("Usage: %s CMD [ARG]\n\n", program);
    std::printf("COMMANDS\n\n");
    for (const auto& [key, cmd] : COMMANDS)
    {
        std::printf("%s %s ", program, key.c_str());

        for (const auto& arg : cmd.usage)
            std::printf("‹%s› ", arg.c_str());
        std::printf("\n");

        for (const auto& line : cmd.desc)
            std::printf("    %s\n", line.c_str());
        std::printf("\n");
    }
}


message cmd_start(const message& msg, server_t& server)
{
    const auto& arg = msg.line(0);
    auto it = server.apps.find(arg);

    if (it == server.apps.end())
        return message{ "error", "invalid app name '%s'", arg };

    char* const* argv = it->second.start.get();
    const auto& dir = it->second.dir;

    auto redir = std::map<int, fs::path>
    {
        { fd_t::fileno(stdout), LOG_PATH / arg += ".stdout.log" },
        { fd_t::fileno(stderr), LOG_PATH / arg += ".stderr.log" },
    };

    auto to_close = std::vector<int>{ };

    const auto& [nit, succ] = server.procs.try_emplace(arg, argv, dir, redir);
    if (!succ)
        return message{ "error", "already running" };

    return message{ "ok", "pid: %d", int(nit->second.pid) };
}


message cmd_stop(const message& msg, server_t& server)
{
    const auto& arg = msg.line(0);

    auto it = server.procs.find(arg);
    if (it == server.procs.end())
        return message{ "error", "not running" };

    // TODO: first attempt to terminate peacefully
    it->second.signal(SIGKILL);
    server.reap(it);

    return message{ "ok", "killed" };
}


message cmd_update(const message& msg, server_t& server)
{
    const auto& arg = msg.line(0);

    auto app_it = server.apps.find(arg);
    if (app_it == server.apps.end())
        return message{ "error", "invalid app name '%s'", arg };

    auto resp = message{};

    auto proc_it = server.procs.find(arg);
    if (proc_it != server.procs.end())
    {
        // TODO: first attempt to terminate peacefully
        proc_it->second.signal(SIGKILL);
        server.reap(proc_it);
        resp.add_line("killed");
    }

    // TODO: interrupt if client dies when updating
    // TODO: pipe update output and send to client
    auto update = proc_t{ app_it->second.update.get(),
                          app_it->second.dir };
    auto ex = update.wait();

    bool ok = false;
    if (const auto* e = std::get_if<e_exit>(&ex))
        ok = (e->ret == 0) || (resp.add_line("exit: %d", e->ret), false);
    else if (const auto* s = std::get_if<e_sig>(&ex))
        resp.add_line("signal: %d", s->sig);

    resp.set_arg(ok ? "ok" : "error");
    return resp;
}


message cmd_list  (const message&, server_t& server)
{
    auto str_exit = [](const auto& ex)
    {
        if (!ex)
            return std::array<char, 256>{ "-" };

        std::array<char, 256> res = { 0 };

        if (const auto* e = std::get_if<e_exit>(&*ex))
        {
            std::snprintf(res.data(), res.size(), "exit %d", e->ret);
        }
        else if (const auto* s = std::get_if<e_sig>(&*ex))
        {
            std::snprintf(res.data(), res.size(),
                         "%s (%d): %s",
                         str_sig(s->sig), s->sig, ::strsignal(s->sig));
        }
        return res;
    };

    auto resp = message{ "ok" };

    resp.add_line("%-20s │ %10s │ %20s", "APP", "PID", "EXIT");
    resp.add_line("%-20s─┼─%10s─┼─%20s", "────────────────────", "──────────",
                                         "────────────────────");
    for (const auto& [key, app] : server.apps)
    {
        auto it = server.procs.find(key);
        if (it != server.procs.end())
        {
            resp.add_line("%-20s │ %10d │ %-20s", key.c_str(), it->second.pid,
                                                 str_exit(app.exit).data());
        }
        else
        {
            resp.add_line("%-20s │ %10s │ %-20s", key.c_str(), "-",
                                                 str_exit(app.exit).data());
        }
    }
    return resp;
}


message cmd_signal(const message& msg, server_t& server)
{
    const auto& sig = msg.line(1);
    const auto& arg = msg.line(0);

    auto it = server.procs.find(arg);
    if (it == server.procs.end())
        return message{ "error", "'%s' not running", arg };

    int s = int_sig(sig);
    if (s == -1)
        return message{ "error", "invalid signal '%s'", sig };

    it->second.signal(s);

    return { "ok" };
}
