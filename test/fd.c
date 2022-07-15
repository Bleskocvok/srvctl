
#include <stdio.h>      // printf
#include <errno.h>      // errno

#include <unistd.h>     // sysconf, write
#include <fcntl.h>      // fcntl


int main()
{
    printf("(fd) start\n");
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

    // char buf[] = { 0 };
    // int r = read(3, buf, sizeof(buf) - 1);
    // if (r == -1)
    //     printf("read: err\n");
    // else if (r >= 0)
    //     printf("read: %*s\n", r, buf);

    fprintf(stderr, "(fd) some error\n");

    printf("(fd) exit\n");
    return 0;
}
