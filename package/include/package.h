#ifndef __PACKAGE_H__
#define __PACKAGE_H__

/*
  接收端和发送端同时包含此模块，即可方便地实现数据包传输。

  包头格式：
    ESC+HEAD      表示包头
    ESC+DAT       表示是数据，数据值就是ESC
  发送端：
    包头    ESC+HEAD
    数据    检测并发送，当检测到数据是ESC时，则发送ESC+DAT
  接收端：
    检测到非ESC     认为是数据
    检测到ESC       进行标记，等待下一个数据
      下一个数据检测到HEAD    认为是包头
      下一个数据检测到DAT     认为是数据ESC
  总体数据格式：
    包头 + 数据包长度高位 + 数据包长度低位 + 数据包
    发送端和接收端在收发数据包长度时也会进行转义字符检测，防止数据正好和转义字符相同

  使用方法：
  1、客制化转义符、包头、数据
  2、客制化发送缓冲区和接收的包缓冲区大小
  3、使用package_set_call_back设置接收到包后的回调函数
  4、package_send 发送包
  5、package_get或package_get_buf 把接收到的数据传给模块，模块解析数据，解析到一个完整包的话则回调函数
 */

#define SEND_BUF_SIZE 1024
#define PACKAGE_BUF_SIZE 1024
#define ESC     0x12    //转义字符
#define DAT     0x00    //数据的转义，ESC+DAT 就表示数据ESC
#define HEAD    0x34    //包头的转义，ESC+HEAD 就表示包头

void package_set_call_back(void (*f)(unsigned char *p, unsigned int len));  //设置回调函数
void package_send(unsigned char *dat, unsigned int len); //发送指定长度数据包

/*接收到的数据传给这个函数，
    可以是串口中断直接调用此函数，但这样可能会导致数据丢失（下个字节到来了还没有处理完）；
    也可以是DMA接收，接收之后迭代进此函数，推荐这种方法。
  此函数会解析数据，当解析到完整的数据包时会回调 package_set_call_back 设置好的函数 */
void package_get(unsigned char get);

/* 上面的package_get需要依次把接收到的各个数据传入，有时候有一个buf的话就要写一个循环，比较麻烦。
   这里直接提供这个方法，传入buf，自动迭代。   */
void package_get_buf(unsigned char *p, unsigned int len);


#endif
