
/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-24     j         keys  init
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* 使用静态方法线程创建*/
//#define STATIC_METHON

#ifndef STATIC_METHON
/* 定义一个按键检测线程句柄结构体指针 */
static rt_thread_t key_thread = RT_NULL;
/* 定义一个按键状态邮箱控制块结构体指针*/
rt_mailbox_t key_mailbox = RT_NULL;
#else
/*定义一个按键检测静态线程栈*/
static char key_thread_stack[256];
/*定义一个按键检测静态线程句柄*/
static struct rt_thread key_thread;
#endif

rt_sem_t ip_setting_sem =RT_NULL;
extern rt_uint8_t SERVER_ADDR[25];

/* 获取相应的引脚编号 */
#define PIN_WK_UP   GET_PIN(C, 13)
#define PIN_KEY0    GET_PIN(D, 10)
#define PIN_KEY1    GET_PIN(D, 9)
#define PIN_KEY2    GET_PIN(D, 8)

#define SETTING_PAGE   1
#define SHOWING_PAGE   0

extern void lcd_fresh();
extern rt_uint8_t MAX_TEMP_STR[3]  ;
extern rt_uint8_t MAX_HUMI_STR[3]  ;
extern rt_uint8_t MAX_TEMP  ;
extern rt_uint8_t MAX_HUMI ;

rt_uint8_t is_shutdown_beep=0;
rt_uint8_t current_page=1;
rt_uint8_t edit_focus=6;
rt_uint8_t is_ready=0;

/* 按键检测线程入口函数*/
static void key_thread_entry(void *parameter)
{
    int num;
    static rt_uint8_t key_up = 1;   /* 按键松开标志 */
    /* 初始化按键 */
    rt_pin_mode(PIN_WK_UP, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY1, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY2, PIN_MODE_INPUT);

    while (1)
    {
        /* 检测按键是否按下 */
        if (key_up && ((rt_pin_read(PIN_WK_UP) == PIN_HIGH) ||
                      (rt_pin_read(PIN_KEY0) == PIN_LOW)    ||
                      (rt_pin_read(PIN_KEY1) == PIN_LOW)    ||
                      (rt_pin_read(PIN_KEY2) == PIN_LOW) )       )
        {
            rt_thread_mdelay(50);   /* 延时消抖*/
            key_up = 0;
            if (rt_pin_read(PIN_WK_UP) == PIN_HIGH)
            {
                if(!is_ready){
                    rt_sem_release(ip_setting_sem);
                    current_page=0;
                    is_ready=1;
                }
                else{
                    MAX_HUMI=(MAX_HUMI_STR[0]-'0')*10+(MAX_HUMI_STR[1]-'0');
                    MAX_TEMP=(MAX_TEMP_STR[0]-'0')*10+(MAX_TEMP_STR[1]-'0');
                }
                edit_focus=0;
                is_shutdown_beep=!is_shutdown_beep;
            }
            else if (rt_pin_read(PIN_KEY0) == PIN_LOW)
            {

                current_page=!current_page;
                //lcd_fresh();
            }
            else if (rt_pin_read(PIN_KEY1) == PIN_LOW)
            {
                if(current_page)
                    edit_focus++;
            }
            else if (rt_pin_read(PIN_KEY2) == PIN_LOW)
            {
                if(!is_ready){
                    num=(int)(SERVER_ADDR[8+edit_focus]-'0');
                    num++;
                    num%=10;
                    SERVER_ADDR[8+edit_focus]=(rt_uint8_t)(num+'0');
                }
                else {
                    if(edit_focus<2){
                        num=(int)(MAX_TEMP_STR[edit_focus]-'0') ;
                        num++;
                        num%=10;
                        MAX_TEMP_STR[edit_focus]=(rt_uint8_t)(num+'0');
                    }
                    else {
                        num=(int)(MAX_HUMI_STR[edit_focus-2]-'0') ;
                        num++;
                        num%=10;
                        MAX_HUMI_STR[edit_focus-2]=(rt_uint8_t)(num+'0');
                    }
                }
            }
        }
        else if((rt_pin_read(PIN_WK_UP) == PIN_LOW) &&
                (rt_pin_read(PIN_KEY0) == PIN_HIGH) &&
                (rt_pin_read(PIN_KEY1) == PIN_HIGH) &&
                (rt_pin_read(PIN_KEY2) == PIN_HIGH)     )
        {
            key_up = 1;     /* 按键已松开 */
        }
        rt_thread_mdelay(100);
    }

}


void app_key_init(void)
{
    rt_err_t rt_err;
#ifndef STATIC_METHON



    ip_setting_sem=rt_sem_create("ip_set_sem",
            0,
            RT_IPC_FLAG_FIFO);


    /* 创建按键检测线程*/
    key_thread = rt_thread_create(  "key thread",       /* 线程的名称 */
                                    key_thread_entry,   /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    256,                /* 线程栈大小，单位是字节  */
                                    5,                  /* 线程的优先级，数值越小优先级越高*/
                                    10);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (key_thread != RT_NULL)
        rt_err = rt_thread_startup(key_thread);
    else
        rt_kprintf("key thread create failure !!! \n");

    /* 判断线程是否创建成功 */
    if( rt_err == RT_EOK)
        rt_kprintf("key thread startup ok. \n");
    else
        rt_kprintf("key thread startup err. \n");

    /* 使用动态创建方法创建一个邮箱 */
    key_mailbox = rt_mb_create ("key mailbox",      /* 邮箱名称 */
                                4,                  /* 邮箱容量,即该邮箱最多可以保存几封邮件 */
                                RT_IPC_FLAG_FIFO);  /* 采用FIFO方式进行线程等待 */
    /* 判断邮箱是否创建成功 */
    if( key_mailbox != RT_NULL)
        rt_kprintf("key mailbox create succeed. \n");
    else
        rt_kprintf("key mailbox create failure. \n");

#else
    /* 初始化按键检测线程，名称是thread2，入口是thread2_entry */
    rt_err = rt_thread_init(&key_thread,                /* 线程句柄 */
                           "key thread",                /* 线程的名称 */
                           key_thread_entry,            /* 线程入口函数 */
                           RT_NULL,                     /* 线程入口函数的参数   */
                           &key_thread_stack[0],        /* 线程栈起始地址*/
                           sizeof(key_thread_stack),    /* 线程栈大小，单位是字节*/
                           5,                           /* 线程的优先级，数值越小优先级越高*/
                           10);                         /* 线程的时间片大小 */
    /* 如果线程创建成功，启动这个线程 */
    if (rt_err == RT_EOK)
        rt_err = rt_thread_startup(&key_thread);
    else
        rt_kprintf("key thread init failure !!! \n");

    /* 判断线程是否启动成功 */
    if( rt_err == RT_EOK)
        rt_kprintf("key thread startup ok. \n");
    else
        rt_kprintf("key thread startup err. \n");
#endif
}

