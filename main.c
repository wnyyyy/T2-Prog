/* Nokia 5110 LCD AVR Library example
 *
 * Copyright (C) 2015 Sergey Denisov.
 * Written by Sergey Denisov aka LittleBuster (DenisovS21@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version 3
 * of the Licence, or (at your option) any later version.
 *
 * Original library written by SkewPL, http://skew.tk
 * Custom char code by Marcelo Cohen - 2021
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "nokia5110.h"

uint8_t glyph[] = { 0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000 };

int main(void)
{
    disp_startup();
    while (1)
    {
    }
}

void disp_startup()
{
    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);
    nokia_lcd_write_string("Tentativas: ", 1);
    nokia_lcd_drawline(0,9,84,9);
    disp_update(0);
}

void disp_update(int tentativas)
{
    char strTentativas[2];
    sprintf(strTentativas, "%d", tentativas);
    nokia_lcd_write_string(strTentativas, 1);
    nokia_lcd_render(); 
}
