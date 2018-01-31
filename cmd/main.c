#include <stdio.h>
#include <unistd.h>
#include <serial.h>
#include <cmd.h>

int fd;

void cmd_call_back(unsigned int cmd, unsigned int len, unsigned char *dat)
{
    unsigned int i;
    printf("%s\n", __func__);
    printf("cmd is 0x%4x\nlen is %d\n", cmd, len);
    for(i=0;i<len;i++) {
        if(i%16==0) printf("\n");
        printf("%2x ", dat[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    unsigned char buf[1024];
    unsigned char buf_in[1024];
    unsigned int i,len=256;
    
    fd = serialOpen("/dev/ttyUSB0", 115200, 8, 1, 'N', 30);
    serialFlush(fd);

    cmd_set_call_back(cmd_call_back);

    for(i=0;i<len;i++) {
        buf[i] = i;
    }
    cmd_send_cmd(0x55, len, buf);

    usleep(500000);

    len = serialDataAvail(fd);
    len = serialRead(fd, buf_in, len);

    cmd_get_buf(buf_in, len);
    
    serialClose(fd);
    return 0;
}
