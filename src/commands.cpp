#include "commands.hpp"


message cmd_start (const message&, server_t&);
message cmd_stop  (const message&, server_t&);
message cmd_update(const message&, server_t&);


extern const std::map<std::string, command> commands =
{
    { "start",  command{ cmd_start,  "" } },
    { "stop",   command{ cmd_stop,   "" } },
    { "update", command{ nullptr,    "" } },
    { "status", command{ nullptr,    "" } },
};


message cmd_start(const message& msg, server_t& server)
{
    auto it = server.apps.find(msg.str_arg());

    if (it == server.apps.end())
        return message{ "error", "invalid app name" };

    char* const* argv = it->second.start.ptrs.data();
    const auto& dir = it->second.dir;
    if (!server.procs.try_emplace(msg.str_arg(), argv, dir).second)
        return message{ "error", "already running" };

    return message{ "success", "" };
}


message cmd_stop(const message& msg, server_t& server)
{
    auto it = server.procs.find(msg.str_arg());
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
