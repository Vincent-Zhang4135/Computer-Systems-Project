#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
    int status;
    int fd = open("sample.txt", S_IRWXU);
    int temp = open("sample.txt", O_APPEND|S_IRWXU);
    int rv = fork();
    if (rv == 0) {
        printf("%d, %d", fd, fd2);
    } else {
        waitpid(-1, &status, 0);
    }

    return 0;
}