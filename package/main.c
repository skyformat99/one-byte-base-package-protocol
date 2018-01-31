#include <stdio.h>
#include <unistd.h>
#include <serial.h>
#include <package.h>

int fd;

void package_call_back(unsigned char *p, unsigned int len)
{
    int i;
    printf("get package call back\n");
    for(i=0;i<len;i++) {
        if(i%16 == 0) printf("\n");
        printf("0x%2x, ", p[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    unsigned char buf[1024];
    unsigned char buf_in[1024]={0};
    unsigned int len = 256;
    unsigned int i;
    fd = serialOpen("/dev/ttyUSB0", 115200, 8, 1, 'N', 30);

    for(i=0;i<len;i++) {
        buf[i] = i;
    }
    serialFlush(fd);
    package_set_call_back(package_call_back);
    package_send(buf, len);
    usleep(500000);

    len = serialDataAvail(fd);
    len = serialRead(fd, buf_in, len);
    
    package_get_buf(buf_in, len);
    
    serialClose(fd);
    return 0;
}
