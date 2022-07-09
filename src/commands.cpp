#include "commands.hpp"


message cmd_start (const message&, server_t&);
message cmd_stop  (const message&, server_t&);
message cmd_update(const message&, server_t&);
message cmd_list  (const message&, server_t&);


extern const std::map<std::string, command> commands =
{
    { "start",  command{ cmd_start,  "" } },
    { "stop",   command{ cmd_stop,   "" } },
    { "update", command{ nullptr,    "" } },
    { "status", command{ nullptr,    "" } },
    { "list",   command{ nullptr,    "" } },
};


message cmd_start(const message& msg, server_t& server)
{
    const auto& arg = msg.line(0);
    auto it = server.apps.find(arg);

    if (it == server.apps.end())
        return message{ "error", "invalid app name" };

    char* const* argv = it->second.start.c_str();
    const auto& dir = it->second.dir;
    const auto& [nit, succ] = server.procs.try_emplace(arg, argv, dir);
    if (!succ)
        return message{ "error", "already running" };

    std::printf("proc: %d\n", int(nit->second.pid));

    return message{ "success", "" };
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


message cmd_list  (const message&, server_t&)
{
    // [](const auto& info, const auto& it)
    // {
    //     std::printf("waited '%s' ", it->first.c_str());
    //     switch (info.index())
    //     {
    //         case 0: {
    //             auto e = std::get<e_exit>(info);
    //             std::printf("(exit %d)\n", e.ret);
    //         } break;
    //         case 1: {
    //             auto s = std::get<e_sig>(info);
    //             std::printf("(sig  %d: %s)\n", s.sig, strsignal(s.sig));
    //         } break;
    //     }
    // };
    return {};
}
