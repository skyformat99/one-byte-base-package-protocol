#include <package.h>

static unsigned char send_buf[SEND_BUF_SIZE]; //发送缓存，包含了包头、长度和数据包
static unsigned int send_buf_len;
static unsigned char package_buf[PACKAGE_BUF_SIZE]; //接收到的包缓冲区，仅包含数据包，不包含包头和长度

static void (*package_call_back)(unsigned char *p, unsigned int len);  //数据包回调函数，当分析到数据包时回调此函数


/*********************** 串口接口函数 ******************/
#include <serial.h>
static void package_uart_send(unsigned char *dat, unsigned int len) //通过串口发送指定长度数据
{
    extern int fd;
    serialWrite(fd, (char*)dat, len);
}
/************************** 缓冲区操作 ******************************/
static void send_buf_clean()
{
    send_buf_len = 0;
}
static void send_buf_add(unsigned char dat)
{
    send_buf[send_buf_len++] = dat;
}
/************************** 把发送缓冲区的数据发送出去 ******************************/
static void package_send_buf()
{
    package_uart_send(send_buf, send_buf_len);
    send_buf_clean();
}

/************************** 接口函数 ******************************/
void package_set_call_back(void (*f)(unsigned char *p, unsigned int len))  //设置回调函数
        //函数格式必须是 void func(unsigned char *p, unsigned int len)
        //当检测到数据包时会回调此函数，p是数据包位置，len是数据包长度，在回调函数中对数据包处理
{
    package_call_back = f;
}
void package_send(unsigned char *dat, unsigned int len) //发送指定长度数据包
{
    unsigned char len0, len1;
    unsigned int i;
    //包头
    send_buf_clean();
    send_buf_add(ESC);
    send_buf_add(HEAD);
    //包长度
    len1 = (unsigned char)(len>>8);     //高8位
    len0 = (unsigned char)(len);        //低8位
    if(len1==ESC) {     //表示数据就是ESC
        send_buf_add(ESC);
        send_buf_add(DAT);
    } else {
        send_buf_add(len1);
    }
    if(len0==ESC) {     //表示数据就是ESC
        send_buf_add(ESC);
        send_buf_add(DAT);
    } else {
        send_buf_add(len0);
    }
    //数据包
    for(i=0;i<len;i++) {
        if(dat[i] == ESC) {     //数据就是ESC
            send_buf_add(ESC);
            send_buf_add(DAT);
        } else {
            send_buf_add(dat[i]);
        }
    }
    //执行发送
    package_send_buf();
}
void package_get(unsigned char get)
/*接收到的数据传给这个函数，
    可以是串口中断直接调用此函数，但这样可能会导致数据丢失（下个字节到来了还没有处理完）；
    也可以是DMA接收，接收之后迭代进此函数，推荐这种方法。
  此函数会解析数据，当解析到完整的数据包时会回调 package_set_call_back 设置好的函数 */
{
    static int step=0;  /*
                          0-没检测到包头，当前等待包头
                          1-已检测到包头，当前接收数据长度高字节
                          2-已检测到数据长度高字节，当前接收数据长度低字节
                          3-已检测到数据长度低字节，当前等待数据
                         */
    static unsigned char flag_esc=0;        //标记是否接已收到ESC，0-否，1-是
    static unsigned char len0, len1;
    static unsigned int len, count;
    switch(step) {
    case 0:     //没检测到包头，当前等待包头
        if(flag_esc) {
            if(get == HEAD) {   //检测到包头
                step = 1;
                flag_esc = 0;
            } else {
                flag_esc = 0;
            }
        } else {
            if(get == ESC) {
                flag_esc = 1;
            } else {
                flag_esc = 0;
            }
        }
        break;
    case 1:     //已检测到包头，当前接收数据长度高字节
        if(flag_esc) {  //刚刚接收到了ESC
            if(get == DAT) {    //转义为ESC，数据正好是ESC
                len1 = ESC;
                step = 2;
                flag_esc = 0;
            } else if(get == HEAD) {    //这是包头的转义，说明有错，重新初始化到包头
                step = 1;
                flag_esc = 0;
            } else {    //其他数据，也不是转义，说明数据还是有问题，从头开始
                step = 0;
                flag_esc = 0;
            }
        } else {        //刚刚没接收到ESC，这是第一次接收
            if(get == ESC) {
                flag_esc = 1;
            } else {        //不是ESC，就是普通数值，这个数值就是数据长度高字节
                len1 = get;
                step = 2;
            }
        }
        break;
    case 2:     //已检测到数据长度高字节，当前接收数据长度低字节
        if(flag_esc) {  //刚刚检测到了ESC
            if(get == DAT) {    //转义为ESC，数据正好是ESC
                len0 = ESC;
                len = ((unsigned int)(len1)<<8) + ((unsigned int)(len0));
                count = 0;
                step = 3;
            } else if(get == HEAD) {    //这是包头的转义，说明有错，重新初始化到包头
                step = 1;
                flag_esc = 0;
            } else {
                step = 0;
                flag_esc = 0;
            }
        } else {        //刚刚没检测到包头，这是第一次接收
            if(get == ESC) {
                flag_esc = 1;
            } else {    //不是ESC，这个数就是长度低8位
                len0 = get;
                len = ((unsigned int)(len1)<<8) + ((unsigned int)(len0));
                count = 0;
                step = 3;
            }
        }
        break;
    case 3:     //已检测到数据长度低字节，当前等待数据
        if(flag_esc) {  //刚刚接收到了ESC
            if(get == DAT) {    //说明刚刚接收到的数据就是ESC
                flag_esc = 0;
                package_buf[count++] = ESC;
            } else if(get == HEAD) {    //这是包头的转义，说明有错，重新初始化到包头
                step = 1;
                flag_esc = 0;
            } else {    //不是任何转义，说明有错，重新开始
                step = 0;
                flag_esc = 0;
            }
        } else {        //刚刚没接收到ESC
            if(get == ESC) {
                flag_esc = 1;
            } else {    //当前数据就是一个有效的包数据
                package_buf[count++] = get;
            }
        }
        //判断是否接收完毕
        if(count == len) {      //接收完毕，数据在package_buf，长度是len，回调函数
            package_call_back(package_buf, len);
            step = 0;
            flag_esc = 0;
        }
        break;
    }
}
void package_get_buf(unsigned char *p, unsigned int len)
  /* 上面的package_get需要依次把接收到的各个数据传入，有时候有一个buf的话就要写一个循环，比较麻烦。
     这里直接提供这个方法，传入buf，自动迭代。   */
{
    unsigned int i;
    for(i=0;i<len;i++) {
        package_get(p[i]);
    }
}


