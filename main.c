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
#include <stdio.h>

#include "nokia5110.h"

char pw_list[4] = {'+', 'o', '$', '*'};

uint8_t glyph[] = { 0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000 };

void disp_update_tentativas(int tentativas);
void disp_startup();
void disp_show_password(char* password);
void disp_update_input();
char* generate_password();

void disp_update_tentativas(int tentativas)
{
    char strTentativas[2];
    nokia_lcd_set_cursor(68,0);
    sprintf(strTentativas, "%d", tentativas);
    nokia_lcd_write_string(strTentativas, 1);
    nokia_lcd_render(); 
}

void disp_startup()
{
    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);
    nokia_lcd_write_string("Tentativas:", 1);
    nokia_lcd_drawline(0,9,84,9);
    disp_update_tentativas(0);
}       

void disp_show_password(char* password)
{
    for (int i = 0; i < 4; i++)
    {
        nokia_lcd_set_cursor(i*20 + 7, 12);
        nokia_lcd_write_char(password[i], 2);
    }    
    
    nokia_lcd_render(); 
}

void disp_update_input(char* input)
{
    for (int i = 0; i < 4; i++)
    {
        nokia_lcd_set_cursor(i*20 + 7, 12);
        nokia_lcd_write_char(input[i], 2);
    }
    
    nokia_lcd_render(); 
}

char* generate_password()
{
    char *pw = malloc(4);
    pw[0] = pw_list[rand() % 4];
    pw[1] = pw_list[rand() % 4];
    pw[2] = pw_list[rand() % 4];
    pw[3] = pw_list[rand() % 4];

    return pw;
}

int main(void)
{
    disp_startup();
    char *input = malloc(4);
    for (int i = 0; i < 4; i++) {input[i] = 'o';}
    
    while (1)
    {
        _delay_ms(1000);
        char* password = generate_password();
        disp_update_input(input);
    }
}
