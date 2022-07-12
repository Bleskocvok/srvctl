#include "commands.hpp"

// cpp
#include <string>       // string
#include <variant>      // get_if


message cmd_start (const message&, server_t&);
message cmd_stop  (const message&, server_t&);
message cmd_update(const message&, server_t&);
message cmd_list  (const message&, server_t&);
message cmd_status(const message&, server_t&);


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
                         { "List each apps loaded from the confifuration file",
                           "If an instance is running, PID is listed.",
                           "If the app had been stopped, information about",
                           "signal/return is listed." } } },
    // TODO:
    // { "status", command{ cmd_status, {}, {} } },
    // { "signal", command{ cmd_status, {}, {} } },
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
    const auto& [nit, succ] = server.procs.try_emplace(arg, argv, dir);
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


message cmd_status(const message&, server_t&)
{
    return {};
}


message cmd_list  (const message&, server_t& server)
{
    auto str_exit = [](const auto& ex) -> std::string
    {
        if (!ex)
            return "-";

        if (const auto* e = std::get_if<e_exit>(&*ex))
        {
            return "(exit " + std::to_string(e->ret) + ")";
        }
        else if (const auto* s = std::get_if<e_sig>(&*ex))
        {
            return "(sig " + std::to_string(s->sig) + ": "
                   + ::strsignal(s->sig) + ")";
        }
        return {};
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
            resp.add_line("%-20s │ %10d │ %20s", key.c_str(), it->second.pid,
                                                 str_exit(app.exit).c_str());
        }
        else
        {
            resp.add_line("%-20s │ %10s │ %20s", key.c_str(), "-",
                                                 str_exit(app.exit).c_str());
        }
    }
    return resp;
}
