/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2019-08-28     WillianChan   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#ifdef BSP_USING_SPI_LCD
#include <drv_lcd.h>
#include <hunnulogo.h>

extern rt_uint8_t MAX_TEMP  ;
extern rt_uint8_t MAX_HUMI  ;

extern rt_uint8_t MAX_TEMP_STR[3]  ;
extern rt_uint8_t MAX_HUMI_STR[3]  ;

extern rt_uint8_t SERVER_ADDR[25];
extern rt_uint8_t edit_focus;
void lcd_fresh(){
    /* 清屏 */
    lcd_clear(WHITE);

    /* 设置背景色和前景色 */
    lcd_set_color(WHITE, BLUE);

    /* 显示 RT-Thread logo */
    lcd_show_image(0, 0, 112, 52, image_hunnulogo);
    
    /* 在 LCD 上显示字符 */
    lcd_show_string(114, 3, 32, " T-H-M");
   //lcd_draw_line(115, 36, 236, 36);
    lcd_show_string(114, 52-17, 16, "201730132018ZWJ");
    lcd_set_color(WHITE, BLACK);
    /* 在 LCD 上画线 */
    lcd_draw_line(0, 53, 240, 53);
}

void lcd_show_page_emergency(rt_uint16_t situation){
    int id=situation>>8;
    int status=situation&0xff;
    rt_uint8_t show_str[50];
    lcd_fresh();

    switch(status){
    case 1:sprintf(show_str,"No.%d:OVERHEAT!",id+1);break;
    case 2:sprintf(show_str,"No.%d:OVERWET!",id+1);break;
    case 3:sprintf(show_str,"No.%d:BOTH OVER!",id+1);break;
    }

    lcd_set_color(WHITE, RED);
    lcd_draw_rectangle(0, 69, 240, 103);
    lcd_show_string(5, 70, 32, show_str);
    lcd_set_color(WHITE,BLACK);

    lcd_show_string(5, 140+18+18+18, 16, "WK_UP:turn off beep");

    return;
}

void lcd_show_page_setting(){
    //lcd_fresh();

    lcd_show_string(5, 55, 16, "Server IP:");
    lcd_show_string(15, 55+18, 16, SERVER_ADDR);

    lcd_show_string(5, 55+18+18, 32, "max temp:");
    lcd_show_string(5, 55+18+18+34, 32, "max humi:");

    lcd_show_string(5+16*10, 55+18+18, 32, MAX_TEMP_STR);
    lcd_show_string(5+16*10, 55+18+18+34, 32, MAX_HUMI_STR);

    edit_focus%=4;

    lcd_set_color(BLACK, WHITE);
    if(edit_focus<2)
        lcd_show_num(5+16*10+16*(edit_focus), 55+18+18, (int)(MAX_TEMP_STR[edit_focus]-'0'), 10, 32);
    else lcd_show_num(5+16*10+16*(edit_focus-2), 55+18+18+34, (int)(MAX_HUMI_STR[edit_focus-2]-'0'), 10, 32);
    lcd_set_color(WHITE, BLACK);

    lcd_show_string(5, 140+16, 16, "key1:move focu");
    lcd_show_string(5, 140+16+18, 16, "key2:change digital");
    lcd_show_string(5, 140+16+18+18, 16, "WK_UP:save revisions");
    lcd_show_string(5, 140+16+18+18+18, 16, "key0:next page");

}

void lcd_show_page_loadding(){

    edit_focus%=7;
    if(edit_focus==3)edit_focus++;

    lcd_show_string(5, 55, 32, "Server IP:");
    lcd_show_string(0, 55+32+8, 16, "192.168.");
    lcd_show_string(8*9, 55+32, 32, (SERVER_ADDR+8));

    lcd_set_color(BLACK, WHITE);
    lcd_show_num(8*9+16*(edit_focus), 55+32, (int)(SERVER_ADDR[8+edit_focus]-'0'), 10, 32);
    lcd_set_color(WHITE, BLACK);

    lcd_show_string(5, 140, 16, "key1:move focu");
    lcd_show_string(5, 140+18, 16, "key2:change digital");
    lcd_show_string(5, 140+18+18, 16, "WK_UP:save revisions");
    lcd_show_string(5, 140+18+18+18, 16, "key0:next page");
}

static int lcd_sample(void)
{
     lcd_fresh();

    
    return RT_EOK;
}
INIT_APP_EXPORT(lcd_sample);
#endif /* BSP_USING_SPI_LCD */
