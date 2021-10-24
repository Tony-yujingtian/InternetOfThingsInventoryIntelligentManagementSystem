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

#include "drv_lcd.h"
#include <string.h>
#include <finsh.h>

#include <sys/socket.h>
#include "netdb.h"
#include "mailbox_msg.h"

rt_uint8_t SERVER_ADDR[25]=   "192.168.001.119";
#define SEVER_PORT  (8089)

#define BUFSZ       (1024)
#define BEEP_PIN    GET_PIN(B, 2)

struct RT_Client{
    int slaveID;
    int socket;
    uint8_t status;
    float temp,humi;
//    char recvbuf[BUFSZ];
//    char sendbuf[BUFSZ];
//    struct sockaddr_in client_addr;
};


extern rt_sem_t ip_setting_sem;
extern rt_mailbox_t aht10_mailbox ;
char recvbuf[BUFSZ] = {0};
char sendbuf[BUFSZ] = {0};
extern int slaveID;
int sock_fd;
struct hostent *host;
struct sockaddr_in server_addr, client_addr;

static rt_thread_t esp8266_thread = RT_NULL;
rt_thread_t emergency_thread = RT_NULL;
rt_uint16_t is_emergency=0;
void emergency_thread_entry(){
    while(1){
        //is_emergency=0;
        recv(sock_fd, recvbuf, sizeof(recvbuf), 0);
        rt_kprintf("recv data!\n");
        if(recvbuf[0]){
            rt_kprintf("is emergency now!\n");
            is_emergency=((recvbuf[1]<<8)|(recvbuf[0]));
        }
        rt_kprintf("emergency runing!\n");
        rt_thread_mdelay(500);
    }

    return ;
}


static void esp8266_thread_entry(){
    float humi,temp;
    int i=0;
    struct msg* msg_data;
    char count=0;

    while (1)
    {
        if (rt_mb_recv(aht10_mailbox, (rt_uint32_t *)&msg_data , RT_WAITING_FOREVER) == RT_EOK)
        {
            if(msg_data->data_ptr[msg_data->data_size-2]==0x0d&&msg_data->data_ptr[msg_data->data_size-1]==0x0a){


                for(i=0;i<msg_data->data_size;i++){
                    sendbuf[i]=msg_data->data_ptr[i];
                }
                sendbuf[0]=count++;
                sendbuf[1]=slaveID;

                send(sock_fd, sendbuf, msg_data->data_size, 0);

            }
        }
        rt_thread_mdelay(500);
    }
}


void app_esp8266_init(){
        /* ����һ��socket��Э���ΪAT Socket Э��ջ��������SOCK_STREAM��TCP���� */
        sock_fd = socket(AF_AT,SOCK_STREAM,0);
        if (sock_fd  == -1){
            rt_kprintf("Socket error\n");
            return ;
        }

       /*�ź���������������IP*/
        rt_sem_take(ip_setting_sem, RT_WAITING_FOREVER);
        lcd_fresh();

        /* ͨ�������������host��ַ��������������������������� */
        host = (struct hostent *) gethostbyname(SERVER_ADDR);

        /* ��ʼ��Ԥ���ӵķ���˵�ַ */

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SEVER_PORT);
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

        /* ���ӵ������� */
        while (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
        {
            rt_kprintf("Connect fail!\n");
            rt_thread_mdelay(300);
        }


        rt_thread_mdelay(300);
        recv(sock_fd, recvbuf, sizeof(recvbuf), 0);
        rt_kprintf("recv my id info\n");

        if(recvbuf[2]==0x0d&&recvbuf[3]==0x0a){
            slaveID=recvbuf[1];
            rt_kprintf("recv slaveID");
        }
        else slaveID=0;
        rt_kprintf("THREADS START RUN\n");

        esp8266_thread = rt_thread_create("esp8266 Client",     /* �̵߳����� */
                                        esp8266_thread_entry, /* �߳���ں��� */
                                        RT_NULL,            /* �߳���ں����Ĳ���   */
                                        2048,                /* �߳�ջ��С����λ���ֽ�  */
                                        20,                 /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                                        10);                /* �̵߳�ʱ��Ƭ��С */
        /* �������߳̿��ƿ飬��������߳� */
        if (esp8266_thread != RT_NULL)
            rt_thread_startup(esp8266_thread);
        else
            rt_kprintf("esp8266 thread create failure !!! \n");

        emergency_thread = rt_thread_create("emergency thread",     /* �̵߳����� */
                                        emergency_thread_entry, /* �߳���ں��� */
                                        RT_NULL,            /* �߳���ں����Ĳ���   */
                                        2048,                /* �߳�ջ��С����λ���ֽ�  */
                                        19,                 /* �̵߳����ȼ�����ֵԽС���ȼ�Խ��*/
                                        10);                /* �̵߳�ʱ��Ƭ��С */
        /* �������߳̿��ƿ飬��������߳� */
        if (emergency_thread != RT_NULL)
            rt_thread_startup(emergency_thread);
        else
            rt_kprintf("emergency thread create failure !!! \n");

     return ;

}




