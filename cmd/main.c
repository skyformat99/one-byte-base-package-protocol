#include <stdio.h>

int fd;

int main(int argc, char *argv[])
{
    fd = serialOpen("/dev/ttyUSB0", 115200, 8, 1, 'N', 30);
    serialFlush(fd);

    serialClose(fd);
    return 0;
}
