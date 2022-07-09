#include "commands.hpp"

// cpp
#include <string>       // string


message cmd_start (const message&, server_t&);
message cmd_stop  (const message&, server_t&);
message cmd_update(const message&, server_t&);
message cmd_status(const message&, server_t&);
message cmd_list  (const message&, server_t&);


extern const std::map<std::string, command> commands =
{
    { "start",  command{ cmd_start,  "" } },
    { "stop",   command{ cmd_stop,   "" } },
    { "update", command{ cmd_update, "" } },
    { "status", command{ cmd_status, "" } },
    { "list",   command{ cmd_list,   "" } },
};


message cmd_start(const message& msg, server_t& server)
{
    const auto& arg = msg.line(0);
    auto it = server.apps.find(arg);

    if (it == server.apps.end())
        return message{ "error", "invalid app name '%s'", arg };

    char* const* argv = it->second.start.c_str();
    const auto& dir = it->second.dir;
    const auto& [nit, succ] = server.procs.try_emplace(arg, argv, dir);
    if (!succ)
        return message{ "error", "already running" };

    std::printf("proc: %d\n", int(nit->second.pid));

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
    it->second.wait();
    server.procs.erase(it);

    return message{ "ok", "killed" };
}


message cmd_update(const message&, server_t&)
{
    return {};
}


message cmd_status(const message&, server_t&)
{
    return {};
}


message cmd_list  (const message&, server_t& server)
{
    auto str_exit = [](const auto& info) -> std::string
    {
        if (!info)
            return "-";

        switch (info->index())
        {
            case 0: {
                auto e = std::get<e_exit>(*info);
                return "(exit " + std::to_string(e.ret) + ")";
            } break;
            case 1: {
                auto s = std::get<e_sig>(*info);
                return "(sig  " + std::to_string(s.sig) + ": " + strsignal(s.sig) + ")";
            } break;
        }
        return {};
    };

    auto resp = message{ "ok" };

    resp.add_line("%-20s ┃ %20s ┃ %20s", "APP", "PID", "EXIT");
    for (const auto& [key, app] : server.apps)
    {
        auto it = server.procs.find(key);
        if (it != server.procs.end())
        {
            resp.add_line("%-20s ┃ %20d ┃ %20s",
                          key.c_str(), it->second.pid,
                          str_exit(app.exit).c_str());
        }
        else
        {
            resp.add_line("%-20s ┃ %20s ┃ %20s",
                          key.c_str(), "-",
                          str_exit(app.exit).c_str());
        }
    }
    return resp;
}
