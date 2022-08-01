
#include <stdio.h>      // printf
#include <errno.h>      // errno

#include <unistd.h>     // sysconf, write
#include <fcntl.h>      // fcntl


int main()
{
    long max = sysconf(_SC_OPEN_MAX);
    for (int fd = 0; fd < max; fd++)
    {
        errno = 0;
        int r = fcntl(fd, F_GETFD);
        int err = errno;
        if (r != -1)
            printf("fd %6d: open\n", fd);
        else if (err == EBADF)
            ; // printf("fd %6d: closed\n", fd);
        else
            printf("fd %6d: error\n", fd);
    }
    return 0;
}
