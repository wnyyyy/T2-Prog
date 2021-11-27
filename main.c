#define __AVR_ATmega328P__
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#include "adc.h"
#include "print.h"
#include "usart.h"
#include "nokia5110.h"

char pw_list[4] = {'+', 'o', '$', '*'};
float deadzone = 0.150f;
int sel_index = 0;

uint8_t glyph[] = { 0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000 };

void disp_update_tentativas(int tentativas);
void disp_show_password(char* password);
void disp_update(char* input, int tentativas);
void disp_update_input(char* input);
void disp_update_selector();
void joystick_command(char command);
void generate_password(char* pw);

void joystick_command(char command)
{
    switch (command)
    {
    case 'e':
        if (sel_index > 0)
            sel_index--;
        else
            sel_index = 3;
        break;

    case 'd':
        if (sel_index < 3)
            sel_index++;
        else
            sel_index = 0;
        break;

    case 'c':
        break;
    
    case 'b':
        break;
    
    default:
        break;
    }
}

void disp_update(char* input, int tentativas)
{
    nokia_lcd_clear();

    disp_update_selector();
    disp_update_tentativas(tentativas);
    disp_update_input(input);

    nokia_lcd_render(); 
}

void disp_update_selector()
{
    printint(sel_index);
    nokia_lcd_set_cursor(sel_index * 20 + 7, 34);
    nokia_lcd_write_char('^', 2);
}

void disp_update_tentativas(int tentativas)
{
    nokia_lcd_set_cursor(0,0);
    nokia_lcd_write_string("Tentativas:", 1);
    nokia_lcd_drawline(0,9,84,9);
    char strTentativas[2];
    nokia_lcd_set_cursor(68,0);
    sprintf(strTentativas, "%d", tentativas);
    nokia_lcd_write_string(strTentativas, 1);
}

void disp_update_input(char* input)
{
    for (int i = 0; i < 4; i++)
    {
        nokia_lcd_set_cursor(i*20 + 7, 18);
        nokia_lcd_write_char(input[i], 2);
    }
    
    nokia_lcd_render(); 
}

void generate_password(char* pw)
{
    pw[0] = pw_list[random() % 4];
    pw[1] = pw_list[random() % 4];
    pw[2] = pw_list[random() % 4];
    pw[3] = pw_list[random() % 4];
}

// Rotina de tratamento da INT0
ISR(INT0_vect)
{
    print("alo");
    _delay_ms(25);
}


int main(void) {

    USART_Init();
    adc_init();

    DDRD &= ~(1 << PD2);
    PORTD = 1 << PD2;
    
    EICRA = (1 << ISC01) | (1 << ISC00);
    EIMSK |= (1 << INT0); 

    sei();

    int tentativas = 0;
    char* password = malloc(4);
    generate_password(password);
    char* input = malloc(4);
    for (int i = 0; i < 4; i++) {input[i] = 'o';}    
    input[1] = '+';
    input[2] = '*';
    input[3] = '$';

    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);
    disp_update(input, tentativas);

    while (1)
    {
    _delay_ms(25);
    adc_set_channel(0);
    float x = adc_read() * 0.0009765625;
    adc_set_channel(1);
    float y = adc_read() * 0.0009765625;

    if (!(x < 1 - deadzone && x > deadzone && y < 1 - deadzone && y > deadzone)) //se não estiver na deadzone
    {
        PORTD |= (1 << PD0);
        if (y < deadzone) //esquerda
        {
            joystick_command('e');
        }
        else if (y > 1 - deadzone) //direita
        {
            joystick_command('d'); 
        }
        else if (x < deadzone) //cima
        {
            PORTD &= ~(1 << PD1); 
            PORTD &= ~(1 << PD2); 
        }
        else if (x > 1 - deadzone) //baixo
        {
            PORTD |= (1 << PD1);
            PORTD &= ~(1 << PD2);  
        }
        //generate_password(password);
        print(password);
        disp_update(password, tentativas);
        _delay_ms(250);
    }    
  }
}
