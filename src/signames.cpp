
#include "signames.hpp"

// posix
#include <signal.h>     // SIG*

// c
#include <cstring>      // strcmp


extern const std::pair<const char*, int> SIGNAMES[] =
{
    { "SIGABRT",    SIGABRT   },
    { "SIGALRM",    SIGALRM   },
    { "SIGBUS",     SIGBUS    },
    { "SIGCHLD",    SIGCHLD   },
    { "SIGCONT",    SIGCONT   },
    { "SIGFPE",     SIGFPE    },
    { "SIGHUP",     SIGHUP    },
    { "SIGILL",     SIGILL    },
    { "SIGINT",     SIGINT    },
    { "SIGKILL",    SIGKILL   },
    { "SIGPIPE",    SIGPIPE   },
    { "SIGPOLL",    SIGPOLL   },
    { "SIGPROF",    SIGPROF   },
    { "SIGQUIT",    SIGQUIT   },
    { "SIGSEGV",    SIGSEGV   },
    { "SIGSTOP",    SIGSTOP   },
    { "SIGTSTP",    SIGTSTP   },
    { "SIGSYS",     SIGSYS    },
    { "SIGTERM",    SIGTERM   },
    { "SIGTRAP",    SIGTRAP   },
    { "SIGTTIN",    SIGTTIN   },
    { "SIGTTOU",    SIGTTOU   },
    { "SIGURG",     SIGURG    },
    { "SIGUSR1",    SIGUSR1   },
    { "SIGUSR2",    SIGUSR2   },
    { "SIGVTALRM",  SIGVTALRM },
    { "SIGXCPU",    SIGXCPU   },
    { "SIGXFSZ",    SIGXFSZ   },
};


const char* str_sig(int s)
{
    for (auto [str, i] : SIGNAMES)
    {
        if (i == s)
            return str;
    }
    return nullptr;
}


int int_sig(const char* sig)
{
    for (auto [str, i] : SIGNAMES)
    {
        if (std::strcmp(str, sig) == 0)
            return i;
    }
    return -1;
}

