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
#include <rtdevice.h>
#include <board.h>

#include "drv_lcd.h"
#include <string.h>
#include <finsh.h>

#include <stdio.h>

#define SERVER_ADDR   "192.168.1.127"
#define SERVER_PORT  (8089)

#define BUFSZ       (1024)
#define BEEP_PIN    GET_PIN(B, 2)

struct RT_Client{
    int slaveID;
    int list_pos;
    uint8_t status;
    int temp,humi;
    int beat;
//    char recvbuf[BUFSZ];
//    char sendbuf[BUFSZ];
//    struct sockaddr_in client_addr;
};

rt_uint8_t is_connected_client[10]={0};


char recvbuf[BUFSZ] = {0};
char sendbuf[BUFSZ] = {0};
static rt_uint8_t slaveID=0;
rt_uint8_t client_count=0;

static rt_thread_t esp8266_thread = RT_NULL;

#define AP_SSID  "426 technology center"
#define AP_PWD   "426426426"

extern rt_uint16_t uart_recv_len;
extern rt_uint8_t  uart_recv_buf[512];
extern rt_uint8_t  uart_send_buf[512];

rt_uint8_t server_IP[20];
struct RT_Client client_struct[10];
static rt_thread_t client_thread  = RT_NULL;
static rt_thread_t monitor_thread=RT_NULL;
rt_thread_t emergency_thread=RT_NULL;
rt_thread_t beat_thread=RT_NULL;
rt_uint8_t is_emergency=0;
int ready_to_fresh_page=0;
extern rt_uint8_t is_shutdown_beep;


void beat_thread_entry(void *parameter){
    /*clear the disconnected client*/
    int i=0;
    while(1){
        for(i=0;i<slaveID;i++){
            if(is_connected_client[i]>0){
                if(client_struct[i].beat==0){
                    rt_kprintf("a client was disconnected\n");
                    is_connected_client[i]=0;
                    client_count--;
                    lcd_fill(0, 89, 240, 220, WHITE);
                }
                else client_struct[i].beat=0;
            }
        }
        rt_thread_mdelay(20000);
    }

}


void emergency_handle(int i){
    int j=0;
    char show_str[80];

    if(!is_shutdown_beep){
        rt_pin_write(BEEP_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(BEEP_PIN, PIN_LOW);

        uart_send_buf[0]=client_struct[i].status;
        uart_send_buf[1]=client_struct[i].slaveID;
        uart_send_buf[2]=0x0d;
        uart_send_buf[3]=0x0a;

        for(j=0;j<slaveID;j++){
            if(is_connected_client[j]>0)
                if(j!=i){
                    esp8266_ATServerSendData(j,4);
                    rt_thread_mdelay(100);
                }
        }
    }

    switch(client_struct[i].status){
        case 1:sprintf(show_str, "No.%d:OVERHEAT!",client_struct[i].slaveID+1);break;
        case 2:sprintf(show_str, "No.%d:OVERWET !",client_struct[i].slaveID+1);break;
        case 3:sprintf(show_str, "No.%d:BOTH OVER",client_struct[i].slaveID+1);break;
        //default:sprintf(show_str, "No.%d:PARAM ERROR!",client_struct[i].slaveID+1);
    }

    lcd_set_color(WHITE, RED);
    lcd_draw_rectangle(0, client_struct[i].list_pos, 240, client_struct[i].list_pos+36);lcd_show_string(5, client_struct[i].list_pos+3, 32, show_str);
    lcd_set_color(WHITE,BLACK);

    if(!is_shutdown_beep)
        lcd_show_string(5,200 , 16, "WK_UP:turn off all the beeps");
    else lcd_show_string(5, 200, 16, "WK_UP:turn on all the beeps ");

}



void emergency_thread_entry(){
    int i,j;


    int normal_count=0;

    while(1){
        normal_count=0;
        for(i=0;i<slaveID;i++){

            if(is_connected_client[i]>0){
                if(client_struct[i].status!=0){
                    is_emergency= 1;
                    ready_to_fresh_page=1;
                    emergency_handle(i);
                }
                else {
                    normal_count++;
                }
            }
        }

        if(normal_count==client_count){
            is_emergency=0;
        }

        rt_kprintf("emergency runing!\n");
        rt_thread_mdelay(500);
    }

    return ;
}


void client_thread_entry( ){

    rt_uint8_t *ptr=0;
    uart_send_buf[2]=0x0d;
    uart_send_buf[3]=0x0a;
    int id;

    beat_thread= rt_thread_create("beat thread",     /* 线程的名称 */
                                    beat_thread_entry, /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    2048,                /* 线程栈大小，单位是字节  */
                                    20,                 /* 线程的优先级，数值越小优先级越高*/
                                    10);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (beat_thread != RT_NULL){
        rt_thread_startup(beat_thread);
    }

    /* 客户端连接的处理 */
    while (1)
    {
        if(uart_recv_len&0x8000){
            uart_recv_buf[uart_recv_len&0x3fff]=0;
            if(ptr=strstr(uart_recv_buf,",CONNECT")){
                id=(int)(*(ptr-1)-'0');
                if(id>=slaveID){
                    slaveID++;
                }
                client_struct[id].slaveID=id;
                client_struct[id].beat=1;
                is_connected_client[id]=1;
                uart_send_buf[0]=0;
                uart_send_buf[1]=id;

                rt_kprintf("recv respond\n");
                rt_thread_mdelay(200);
                esp8266_ATServerSendData(id,4);
                rt_kprintf("sent first\n");
                rt_thread_mdelay(200);
                esp8266_ATServerSendData(id,4);
                rt_kprintf("sent second\n");
                //slaveID++;
            }
            else if(ptr=strstr(uart_recv_buf,"+IPD")){
               ptr+=9;
               if(!(ptr[7]==0x0d&&ptr[8]==0x0a)){
                   rt_kprintf("wrong data!\n");
               }
               client_struct[ptr[1]].humi= ((ptr[3]<<8)|ptr[4]);
               client_struct[ptr[1]].temp= ((ptr[5]<<8)|ptr[6]);
               client_struct[ptr[1]].status=ptr[2];
               client_struct[ptr[1]].beat++;
            }

            uart_recv_len=0;
        }

        rt_thread_mdelay(500);
    }
    return ;
}

void monitor_thread_entry(){
    int i=0;
    char show_str[80];
    int x,y;
    lcd_fresh( );

    lcd_draw_rectangle(0, 55, 70, 88);lcd_show_string(5, 56, 32, " ID");
    lcd_draw_rectangle(70, 55, 240, 88);lcd_show_string(75, 56, 32, "  PARAM");
    sprintf(show_str,"ip:%s",server_IP);
    lcd_show_string(10, 220, 16, show_str);

    while(1){

        if(ready_to_fresh_page&&is_emergency==0){
            ready_to_fresh_page=0;
            lcd_fill(0, 89, 240, 220, WHITE);
        }

        y=88;
        for(i=0;i<slaveID;i++){
            if(is_connected_client[i]>0){
                sprintf(show_str,"No.%d:humi:%d.%d temp:%d.%d  ",
                        client_struct[i].slaveID+1,
                        client_struct[i].humi/10,client_struct[i].humi%10,
                        client_struct[i].temp/10,client_struct[i].temp%10);

                if(client_struct[i].status==0){//紧急情况不跟新具体数据

                    sprintf(show_str,"No.%d",client_struct[i].slaveID+1);
                    lcd_draw_rectangle(0, y, 70, y+36);lcd_show_string(5, y+3, 32, show_str);

                    sprintf(show_str, " humi |  %d.%d %%",client_struct[i].humi/10,client_struct[i].humi%10);
                    lcd_draw_rectangle(70, y, 240,y+18);lcd_show_string(75, y+1, 16, show_str);

                    sprintf(show_str, " temp |  %d.%d'C",client_struct[i].temp/10,client_struct[i].temp%10);
                    lcd_draw_rectangle(70, y+18, 240,y+18+18);lcd_show_string(75, y+18+1, 16, show_str);
                }
                else {
                    lcd_fill(0, y+2, 240, y+36, WHITE);
                }
                //lcd_draw_line(0, y+18, 240, y+18);
                client_struct[i].list_pos=y;
                y+=36;
            }
        }
        //rt_kprintf("monitor runing!\n");
        rt_thread_mdelay(500);
    }


    return ;
}
void app_esp8266_server_init( )
{
    rt_pin_mode(BEEP_PIN, PIN_MODE_OUTPUT);

    //检测8266
    while(esp8266_ATTest()){
        esp8266_QuitTrans(1);
        lcd_show_string(10,100,16,"esp8266 error!");
        rt_thread_mdelay(500);
    }

    //设置网络模式
    while(esp8266_SetCWMode(1)){
        lcd_show_string(10,100, 16,"CWMODE SETTING...");
        rt_thread_mdelay(100);
    }
    while(esp8266_WIFIConnect(AP_SSID, AP_PWD,0)){
        lcd_show_string(10,100, 16,"ACCESSING AP...          ");
        rt_thread_mdelay(500);
    }
    lcd_show_string(10,100, 16,"FINISHED ACCESSED AP    ");

    //8266重启
    esp8266_DeviceReset();
    lcd_show_string(10,100,16,"WAIT FOR SECONDS...      ");
    rt_thread_mdelay(3000);

    while(esp8266_HasConnectedWIFI()){
        rt_kprintf("has connect wifi?\n");
        rt_thread_mdelay(200);
    }


    esp8266_askForIP(server_IP );

    esp8266_setTCPServer(SERVER_PORT);

    monitor_thread  = rt_thread_create("monitor thread",     /* 线程的名称 */
                                    monitor_thread_entry, /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    4100,                /* 线程栈大小，单位是字节  */
                                    15,                 /* 线程的优先级，数值越小优先级越高*/
                                    10);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (monitor_thread != RT_NULL){
        rt_thread_startup(monitor_thread);
    }
    else
        rt_kprintf("monitor thread create failure !!! \n");

    emergency_thread  = rt_thread_create("emergency thread",     /* 线程的名称 */
                                    emergency_thread_entry, /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    2048,                /* 线程栈大小，单位是字节  */
                                    17,                 /* 线程的优先级，数值越小优先级越高*/
                                    10);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (emergency_thread != RT_NULL){
        rt_thread_startup(emergency_thread);
    }
    else
        rt_kprintf("emergency thread create failure !!! \n");

    client_thread = rt_thread_create("client thread",     /* 线程的名称 */
                                    client_thread_entry, /* 线程入口函数 */
                                    RT_NULL,            /* 线程入口函数的参数   */
                                    1024,                /* 线程栈大小，单位是字节  */
                                    20,                 /* 线程的优先级，数值越小优先级越高*/
                                    10);                /* 线程的时间片大小 */
    /* 如果获得线程控制块，启动这个线程 */
    if (client_thread  != RT_NULL){
        rt_thread_startup(client_thread );
    }
    else
        rt_kprintf("client thread create failure !!! \n" );

    return ;
}








