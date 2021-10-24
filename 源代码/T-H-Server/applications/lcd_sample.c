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


void lcd_fresh(){
    /* ���� */
    lcd_clear(WHITE);

    /* ���ñ���ɫ��ǰ��ɫ */
    lcd_set_color(WHITE, BLUE);

    /* ��ʾ RT-Thread logo */
    lcd_show_image(0, 0, 112, 52, image_hunnulogo);
    
    /* �� LCD ����ʾ�ַ� */
    lcd_show_string(114, 3, 32, " T-H-M");
   //lcd_draw_line(115, 36, 236, 36);
    lcd_show_string(114, 52-17, 16, "201730132018ZWJ");
    lcd_set_color(WHITE, BLACK);
    /* �� LCD �ϻ��� */
    lcd_draw_line(0, 53, 240, 53);
}

static int lcd_sample(void)
{
    lcd_fresh();
    return RT_EOK;
}
INIT_APP_EXPORT(lcd_sample);
#endif /* BSP_USING_SPI_LCD */
