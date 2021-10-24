/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <sys/socket.h>
#include "netdb.h"

#include "drv_lcd.h"


#define LED0_PIN    GET_PIN(E, 7)
#define BEEP_PIN    GET_PIN(B, 2)

void app_key_init(void);
void app_esp8266_init();

extern rt_mailbox_t key_mailbox;
extern rt_mailbox_t aht10_mailbox ;
int main(void){
    int count = 1;

    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

   app_key_init();

   app_esp8266_init();

    while (count++){
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}
