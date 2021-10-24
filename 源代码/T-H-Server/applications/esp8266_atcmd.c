/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-27     j       the first version
 */

/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-26     j       the first version
 */



#include <rtthread.h>
#include <string.h>
#include <stdio.h>

typedef rt_uint8_t u8;
typedef rt_uint16_t u16;
typedef rt_uint32_t u32;

extern rt_device_t esp8266_serial;
extern rt_uint16_t uart_recv_len;
extern rt_uint8_t  uart_recv_buf[512];
extern rt_uint8_t  uart_send_buf[512];


/* 发送字符串 */
rt_size_t rt_device_write(rt_device_t dev,
                          rt_off_t    pos,
                          const void *buffer,
                          rt_size_t   size);


void usart3_senddata(int len){
    rt_device_write(esp8266_serial, 0, uart_send_buf, len);
}


//return 0: correct
u8 esp8266_CheckRespond(u8* exceptRsp,u16 waitTime){
    u8 rst=1;
    while(waitTime--){
        rt_thread_mdelay(10);
      if(uart_recv_len&0x8000){
            uart_recv_buf[uart_recv_len&0x3fff]=0;
            if(strstr((rt_uint8_t *)uart_recv_buf,(rt_uint8_t *)exceptRsp)){
                    rst=0;
            }
            uart_recv_len=0;
            break;
        }
    }

   return rst;

}

u8 esp8266_SendCmd(u8 *cmd,u8 *respond,u16 waitTime){
     int j;
     u16 len=strlen(cmd);

     for(j=0;j<len;j++){
         uart_send_buf[j]=cmd[j];
     }
     uart_send_buf[len++]='\r';
     uart_send_buf[len++]='\n';
     usart3_senddata(len);


     return respond?esp8266_CheckRespond(respond,waitTime):0;

}

u8 esp8266_DeviceReset(){
        u8 rst=esp8266_SendCmd("AT+RST","OK",50);
        rt_thread_mdelay(2000);
        return rst;
}


u8 esp8266_ATTest(){
        return esp8266_SendCmd("AT","OK",50);
}


//#define CWMODE_STA  1
//#define CWMODE_AP   2
//#define CWMODE_STA_AP  3
u8 esp8266_SetCWMode(u8 mode){
      u8 *ATCmd=0;
      u8 rst=0;
      ATCmd=malloc(12);

      if(mode>3)return 1;
      sprintf(ATCmd,"AT+CWMODE=%d",mode);
      rst=esp8266_SendCmd(ATCmd,"OK",50);
    free(ATCmd);
    return rst;
}


u8 esp8266_WIFIConnect(u8 *APName,u8* APPwd,u8 *Encry){
      u8 *ATCmd=0;
      u8 rst=0;
      ATCmd=rt_malloc( 60);

      sprintf(ATCmd,"AT+CWJAP=\"%s\",\"%s\"",APName,APPwd/*,Encry*/);

    rst=esp8266_SendCmd(ATCmd,"OK",60);
        rt_free( ATCmd);
        return rst;
}


u8 esp8266_TCPConnectServer(u8 *IPAddr,u8 * com){
      u8 *ATCmd=0;
      u8 rst=0;
      ATCmd= malloc( 60);

      sprintf(ATCmd,"AT+CIPSTART=\"TCP\",\"%s\",%s",IPAddr,com );
    rst=esp8266_SendCmd(ATCmd,"OK",30);
         free( ATCmd);
        return rst;

}

u8 esp8266_setTCPServer(rt_uint32_t  port){
      u8 *ATCmd=0;
      ATCmd= malloc( 25);
      sprintf(ATCmd,"AT+CIPSERVER=1,%d",port);

      while(esp8266_SendCmd("AT+CIPMODE=0", "OK", 50));

      //多连接
      while(esp8266_SendCmd("AT+CIPMUX=1", "OK", 50));

      //启动server
      while(esp8266_SendCmd(ATCmd, "OK", 50));

      free( ATCmd);
      return 0;
}

u8 esp8266_askForIP(rt_uint8_t * selfIP ){
      u8 *start=0;
      u8 *end=0;

     //selfIP=malloc(20);

      esp8266_SendCmd("AT+CIFSR", 0, 50);

      while(!(uart_recv_len&0x8000)){

          rt_kprintf("wait for my ip\n");
                  rt_thread_mdelay(200);
      }

      uart_recv_buf[uart_recv_len&0x3fff]=0;

      start=strstr(uart_recv_buf,"STAIP,\"");
      end=strstr(uart_recv_buf,"\"\r\n+CIFSR:STAMAC");

      if(start!=NULL)start+=7;
      (*end)=0;

      rt_kprintf("my ip:%s\n",start);

      strcpy(selfIP,start);

      //free(selfIP);

      return 0;
}


u8 esp8266_EnterTrans(){
   return esp8266_SendCmd("AT+CIPMODE=1","OK",50);
}

u8 esp8266_EnableTrans(){
   return esp8266_SendCmd("AT+CIPSEND","OK",50);
}

u8 esp8266_QuitTrans(u8 mode){

    rt_thread_mdelay(100);
    rt_device_write(esp8266_serial, 0, (rt_uint8_t*)&"+++", 3);
    rt_thread_mdelay(300);
    return mode?esp8266_SendCmd("AT+CIPMODE=0","OK",50):0;
}

u8 esp8266_ATServerSendData(rt_uint32_t id,u16 len){
      u8 *ATCmd=0;
      u8 temp[50];
      u8 i=0;
      ATCmd=malloc(50);
      sprintf(ATCmd,"AT+CIPSEND=%d,%d",id,len );

      rt_uint32_t cmdLen=strlen(ATCmd);

      for(i=0;i<cmdLen+3;i++){
          temp[i]=uart_send_buf[i];
      }
      esp8266_SendCmd(ATCmd,"OK",10);
      for(i=0;i<cmdLen+3;i++){
          uart_send_buf[i]=temp[i];
      }
      usart3_senddata( len);
      free(ATCmd);
      return esp8266_CheckRespond("OK",20);
}

u8 esp8266_ATSendData(u16 len){
      u8 *ATCmd=0;
      u8 temp[50];
      u8 i=0;
      ATCmd=malloc(50);
      sprintf(ATCmd,"AT+CIPSEND=%d",len );

      rt_uint32_t cmdLen=strlen(ATCmd);

      for(i=0;i<cmdLen+3;i++){
          temp[i]=uart_recv_buf[i];
      }

    esp8266_SendCmd(ATCmd,"OK",10);

    for(i=0;i<cmdLen+3;i++){
        uart_recv_buf[i]=temp[i];
    }

    usart3_senddata( len);
        free(ATCmd);
        return esp8266_CheckRespond("OK",20);
}

u8 esp8266_HasConnectedWIFI(){
        return esp8266_SendCmd("AT+CWJAP?","OK",20);
}



