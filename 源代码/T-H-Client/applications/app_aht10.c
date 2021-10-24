/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-24     j       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>
#include <string.h>

#include "aht10.h"
#include "drv_lcd.h"
#include "mailbox_msg.h"

#define SETTING_PAGE   1
#define SHOWING_PAGE   0


rt_uint8_t MAX_TEMP = 40;
rt_uint8_t MAX_HUMI = 87;
rt_uint8_t MAX_TEMP_STR[3]  ;
rt_uint8_t MAX_HUMI_STR[3]  ;
#define BEEP_PIN    GET_PIN(B, 2)

 int slaveID=-1;
extern struct aht10_device *temp_humi_dev;
extern rt_uint8_t current_page;
rt_uint8_t prepage=1;
extern rt_uint8_t is_ready;
extern rt_uint8_t is_emergency ;
/* ����һ����ʪ�Ȳɼ��߳̾���ṹ��ָ�� */
static rt_thread_t aht10_thread = RT_NULL;
rt_uint8_t ready_to_fresh=0;
rt_mailbox_t aht10_mailbox = RT_NULL;
rt_uint8_t is_shutdown_beep;


struct msg* msg_ptr;

rt_uint8_t check_temphumi(float t,float h){
    if(!(t>MAX_TEMP||h>MAX_HUMI))
        return 0;
    if(t>MAX_TEMP&&h>MAX_HUMI)return 3;
    if(t>MAX_TEMP)return 1;
    if(h>MAX_HUMI)return 2;
}


/* ��ʪ�Ȳɼ��߳���ں���*/
static void aht10_thread_entry(void *parameter)
{
    rt_uint8_t  frame[9];
    char str[30];
    float humidity, temperature;
    rt_uint8_t status=0;
    int humi,temp;

    msg_ptr=(struct msg*)rt_malloc(sizeof(struct msg));
    msg_ptr->data_ptr=frame;
    msg_ptr->data_size=9;


    while (1)
    {
        frame[7]=0x0d;
        frame[8]=0x0a;



        /* read humidity �ɼ�ʪ�� */
        humidity = aht10_read_humidity(temp_humi_dev);
        rt_kprintf("humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10); /* former is integer and behind is decimal */

        /* read temperature �ɼ��¶� */
        temperature = aht10_read_temperature(temp_humi_dev);

        if(current_page!=prepage){
            lcd_fresh();
            prepage=current_page;
            sprintf(MAX_TEMP_STR,"%d",MAX_TEMP);
            sprintf(MAX_HUMI_STR,"%d",MAX_HUMI);
        }

        if(ready_to_fresh&&is_emergency==0){
            lcd_fresh();
            ready_to_fresh=0;
        }

        if(is_emergency){
            lcd_show_page_emergency(is_emergency);
            is_emergency=0;
            ready_to_fresh=1;


            if(!is_shutdown_beep){
                    rt_pin_write(BEEP_PIN, PIN_HIGH);
                    rt_thread_mdelay(500);
                    rt_pin_write(BEEP_PIN, PIN_LOW);
            }
        }
        else if(current_page==SHOWING_PAGE){
            ready_to_fresh=0;
            if(slaveID>=0){
                sprintf(str,"T-H-M No.%d",slaveID+1);
                lcd_show_string(10, 60, 32, str);
            }

            sprintf(str,"humi:%d.%d %%",(int)humidity, (int)(humidity * 10) % 10);
            lcd_show_string(10, 100, 32,str);
            sprintf(str,"temp:%d.%d'C",(int)temperature, (int)(temperature * 10) % 10);
            lcd_show_string(10, 140, 32,str);

            lcd_show_string(5, 140+18+18+18, 16, "key0:setting page");
        }
        else {
            if(is_ready){
                lcd_show_page_setting();
            }else{
                lcd_show_page_loadding();
            }

        }

        if(status=check_temphumi(temperature, humidity)){
            if(!is_shutdown_beep){
                    rt_pin_write(BEEP_PIN, PIN_HIGH);
                    rt_thread_mdelay(500);
                    rt_pin_write(BEEP_PIN, PIN_LOW);
            }
        }

        humi=(int)(humidity*10);
        temp=(int)(temperature*10);
        rt_kprintf("humi:%d temp:%d\n",humi,temp);

        frame[2]=status;
        frame[3]=(rt_uint8_t)((humi>>8)&0xff);
        frame[4]=(rt_uint8_t)humi&0xff;

        frame[5]=(rt_uint8_t)(temp>>8)&0xff;
        frame[6]=(rt_uint8_t)temp&0xff;

        rt_mb_send(aht10_mailbox, (rt_uint32_t)msg_ptr);

        rt_thread_mdelay(1000);
    }

}


static int app_aht10_init(void)
{
    rt_err_t rt_err;

    rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);

    /* ������ʪ�Ȳɼ��߳�*/
    aht10_thread = rt_thread_create("aht10 thread",     /* �̵߳����� */
                                    aht10_thread_entry, /* �߳���ں��� */
                                    RT_NULL,            /* �߳���ں����Ĳ���   */
                                    2048,                /* �߳�ջ��С����λ���ֽ�  */
                                    25,                 /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                                    10);                /* �̵߳�ʱ��Ƭ��С */
    /* �������߳̿��ƿ飬��������߳� */
    if (aht10_thread != RT_NULL)
        rt_err = rt_thread_startup(aht10_thread);
    else
        rt_kprintf("aht10 thread create failure !!! \n");

    /* �ж��߳��Ƿ񴴽��ɹ� */
    if( rt_err != RT_EOK)
        rt_kprintf("aht10 thread startup err. \n");


    /* ʹ�ö�̬������������һ������ */
    aht10_mailbox = rt_mb_create ("aht10 mailbox",      /* �������� */
                                3,                  /* ��������,�������������Ա��漸���ʼ� */
                                RT_IPC_FLAG_FIFO);  /* ����FIFO��ʽ�����̵߳ȴ� */
    /* �ж������Ƿ񴴽��ɹ� */
    if( aht10_mailbox != RT_NULL)
        rt_kprintf("key mailbox create succeed. \n");
    else
        rt_kprintf("key mailbox create failure. \n");

    return RT_EOK;
}

INIT_APP_EXPORT(app_aht10_init);
