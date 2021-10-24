/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-27     j       the first version
 */


#include <rtthread.h>
//#include "mydevice_uart3.h"

#define  MY_UART_NAME       "uart2"      /* 串口设备名称 */

/* 用于接收消息的信号量 */
struct rt_semaphore rx_sem;
rt_device_t esp8266_serial;
rt_uint16_t uart_recv_len=0;
rt_uint8_t  uart_recv_buf[512];
rt_uint8_t  uart_send_buf[512];

static rt_timer_t recv_timer;
static void recv_timeout(void *parameter);

void recv_timeout(void *parameter){
   // rt_kprintf("stop recieving !\n" );
    //结束接收
    uart_recv_len|=1<<15;
    uart_recv_buf[uart_recv_len&0x3fff]=0;
    rt_kprintf("recv:%s\n",uart_recv_buf);
    recv_timer=RT_NULL;
}



/* 接收数据回调函数 */
 static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
     //rt_kprintf("recving callback!\n" );
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    if(!(uart_recv_len&0x8000))
        rt_sem_release(&rx_sem);

    return RT_EOK;
}

void serial_thread_entry(void *parameter)
{
    char ch;
    rt_kprintf("running uart thread!\n" );
    while (1)
    {
        //rt_kprintf("while uart thread!\n" );
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (rt_device_read(esp8266_serial, -1, &ch, 1) != 1)
        {
            //rt_kprintf("going to semaphore!\n" );
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);

            //rt_kprintf("semaphore running!\n" );
            if(recv_timer==RT_NULL){
                recv_timer= rt_timer_create("timer2",recv_timeout,RT_NULL,15,RT_TIMER_FLAG_ONE_SHOT);
                rt_timer_start(recv_timer);
            }
            //if (recv_timer != RT_NULL)


        }
        //rt_kprintf("recieve data!\n" );
        /* 读取到的数据通过串口错位输出 */
        uart_recv_buf[uart_recv_len++]=ch;

    }
}

int uart_init( )
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    char str[] = "hello RT-Thread!\r\n";


    rt_strncpy(uart_name, MY_UART_NAME, RT_NAME_MAX);


    /* 查找串口设备 */
    esp8266_serial = rt_device_find(uart_name);
    while (!esp8266_serial)
    {
        rt_kprintf("find uart3 failed!\n" );
        return RT_ERROR;
    }

    /* 初始化信号量 */
    while(rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO)!=RT_EOK){
        rt_kprintf("semaphore init!\n" );
    }
    /* 以读写及中断接收方式打开串口设备 */
    rt_device_open(esp8266_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(esp8266_serial, uart_input);


    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 15, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL){
        rt_thread_startup(thread);

    }
    else{
        rt_kprintf("error uart thread!\n" );
        ret = RT_ERROR;
    }

    return ret;
}

 /* APPLICATIONS_MYDEVICE_UART3_C_ */

