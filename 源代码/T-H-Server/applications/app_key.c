
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

/* ʹ�þ�̬�����̴߳���*/
//#define STATIC_METHON

#ifndef STATIC_METHON
/* ����һ����������߳̾���ṹ��ָ�� */
static rt_thread_t key_thread = RT_NULL;

#else
/*����һ��������⾲̬�߳�ջ*/
static char key_thread_stack[256];
/*����һ��������⾲̬�߳̾��*/
static struct rt_thread key_thread;
#endif



/* ��ȡ��Ӧ�����ű�� */
#define PIN_WK_UP   GET_PIN(C, 13)
#define PIN_KEY0    GET_PIN(D, 10)
#define PIN_KEY1    GET_PIN(D, 9)
#define PIN_KEY2    GET_PIN(D, 8)



extern void lcd_fresh();

rt_uint8_t is_shutdown_beep=0;

/* ��������߳���ں���*/
static void key_thread_entry(void *parameter)
{
    int num;
    static rt_uint8_t key_up = 1;   /* �����ɿ���־ */
    /* ��ʼ������ */
    rt_pin_mode(PIN_WK_UP, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY1, PIN_MODE_INPUT);
    rt_pin_mode(PIN_KEY2, PIN_MODE_INPUT);

    while (1)
    {
        /* ��ⰴ���Ƿ��� */
        if (key_up && ((rt_pin_read(PIN_WK_UP) == PIN_HIGH) ||
                      (rt_pin_read(PIN_KEY0) == PIN_LOW)    ||
                      (rt_pin_read(PIN_KEY1) == PIN_LOW)    ||
                      (rt_pin_read(PIN_KEY2) == PIN_LOW) )       )
        {
            rt_thread_mdelay(50);   /* ��ʱ����*/
            key_up = 0;
            if (rt_pin_read(PIN_WK_UP) == PIN_HIGH)
            {
                is_shutdown_beep=!is_shutdown_beep;
            }
            else if (rt_pin_read(PIN_KEY0) == PIN_LOW)
            {

            }
            else if (rt_pin_read(PIN_KEY1) == PIN_LOW)
            {

            }
            else if (rt_pin_read(PIN_KEY2) == PIN_LOW)
            {

            }
        }
        else if((rt_pin_read(PIN_WK_UP) == PIN_LOW) &&
                (rt_pin_read(PIN_KEY0) == PIN_HIGH) &&
                (rt_pin_read(PIN_KEY1) == PIN_HIGH) &&
                (rt_pin_read(PIN_KEY2) == PIN_HIGH)     )
        {
            key_up = 1;     /* �������ɿ� */
        }
        rt_thread_mdelay(100);
    }

}


void app_key_init(void)
{
    rt_err_t rt_err;
#ifndef STATIC_METHON


    /* ������������߳�*/
    key_thread = rt_thread_create(  "key thread",       /* �̵߳����� */
                                    key_thread_entry,   /* �߳���ں��� */
                                    RT_NULL,            /* �߳���ں����Ĳ���   */
                                    256,                /* �߳�ջ��С����λ���ֽ�  */
                                    5,                  /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                                    10);                /* �̵߳�ʱ��Ƭ��С */
    /* �������߳̿��ƿ飬��������߳� */
    if (key_thread != RT_NULL)
        rt_err = rt_thread_startup(key_thread);
    else
        rt_kprintf("key thread create failure !!! \n");

    /* �ж��߳��Ƿ񴴽��ɹ� */
    if( rt_err == RT_EOK)
        rt_kprintf("key thread startup ok. \n");
    else
        rt_kprintf("key thread startup err. \n");


#else
    /* ��ʼ����������̣߳�������thread2�������thread2_entry */
    rt_err = rt_thread_init(&key_thread,                /* �߳̾�� */
                           "key thread",                /* �̵߳����� */
                           key_thread_entry,            /* �߳���ں��� */
                           RT_NULL,                     /* �߳���ں����Ĳ���   */
                           &key_thread_stack[0],        /* �߳�ջ��ʼ��ַ*/
                           sizeof(key_thread_stack),    /* �߳�ջ��С����λ���ֽ�*/
                           5,                           /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                           10);                         /* �̵߳�ʱ��Ƭ��С */
    /* ����̴߳����ɹ�����������߳� */
    if (rt_err == RT_EOK)
        rt_err = rt_thread_startup(&key_thread);
    else
        rt_kprintf("key thread init failure !!! \n");

    /* �ж��߳��Ƿ������ɹ� */
    if( rt_err == RT_EOK)
        rt_kprintf("key thread startup ok. \n");
    else
        rt_kprintf("key thread startup err. \n");
#endif
}

