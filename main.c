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
int started = 0;
uint16_t tic = 0;
char *password;
int tentativas = 0;
char *input;
int input_controller[4] = {0, 0, 0, 0};

uint8_t glyph[] = {0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000};

void disp_update_tentativas();
void disp_show_password();
void disp_update();
void disp_update_input();
void disp_update_selector();
void joystick_command(char command);
void generate_password();
void enter_attempt();

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
        if (input_controller[sel_index] < 3)
        {
            input_controller[sel_index]++;
        }
        else
        {
            input_controller[sel_index] = 0;
        }
        input[sel_index] = pw_list[input_controller[sel_index]];

        break;

    case 'b':
        if (input_controller[sel_index] > 0)
        {
            input_controller[sel_index]--;
        }
        else
        {
            input_controller[sel_index] = 3;
        }
        input[sel_index] = pw_list[input_controller[sel_index]];

        break;

    default:
        break;
    }
}

void enter_attempt()
{
    int acertos = 0;
    int simbolos = 0;
    int qtd_senha[4] = {0,0,0,0};
    int qtd_input[4] = {0,0,0,0};    

    for (int i = 0; i < 4; i++)
    {
        for (int c = 0; c < 4; c++)
        {
           if (password[i] == pw_list[c])
           {
               qtd_senha[c]++;
           }
        }
        for (int c = 0; c < 4; c++)
        {
           if (input[i] == pw_list[c])
           {
               qtd_input[c]++;
           }
        }

        if (input[i] == password[i])
        {
            acertos++;
        }        
    }

    for (int i = 0; i < 4; i++)
    {
        if (qtd_input[i] >= qtd_senha[i])
        {
            simbolos += qtd_senha[i];
        }
        else
        {
            simbolos += qtd_input[i];
        }
    }

    print("\n");
    print("\n");
    print("acertos: ");
    printint(acertos);
    print("\n");
    print("simbolos corretas: ");
    printint(simbolos);
}

void disp_update()
{
    nokia_lcd_clear();

    disp_update_selector();
    disp_update_tentativas();
    disp_update_input();

    nokia_lcd_render();
}

void disp_update_selector()
{
    nokia_lcd_set_cursor(sel_index * 20 + 7, 34);
    nokia_lcd_write_char('^', 2);
}

void disp_update_tentativas()
{
    nokia_lcd_set_cursor(0, 0);
    nokia_lcd_write_string("Tentativas:", 1);
    nokia_lcd_drawline(0, 9, 84, 9);
    char strTentativas[2];
    nokia_lcd_set_cursor(68, 0);
    sprintf(strTentativas, "%d", tentativas);
    nokia_lcd_write_string(strTentativas, 1);
}

void disp_update_input()
{
    for (int i = 0; i < 4; i++)
    {
        nokia_lcd_set_cursor(i * 20 + 7, 18);
        nokia_lcd_write_char(input[i], 2);
    }
}

void generate_password()
{
    print("\nseed: ");
    printint(tic);

    srandom(tic);
    password[0] = pw_list[random() % 4];
    password[1] = pw_list[random() % 4];
    password[2] = pw_list[random() % 4];
    password[3] = pw_list[random() % 4];

    print("\npassword: ");
    print(password);
}

// Rotina de tratamento da INT0
ISR(INT0_vect)
{
    if (started == 0)
    {
        generate_password();
        started = 1;
        disp_update();
    }
    else
    {
        enter_attempt();
    }
    _delay_ms(25);
}

int main(void)
{
    USART_Init();
    adc_init();

    DDRD &= ~(1 << PD2);
    PORTD = 1 << PD2;
    EICRA = (1 << ISC01) | (1 << ISC00);
    EIMSK |= (1 << INT0);

    sei();

    tentativas = 0;
    password = malloc(4);
    input = malloc(4);
    char first_char = pw_list[0];
    input[0] = first_char;
    input[1] = first_char;
    input[2] = first_char;
    input[3] = first_char;

    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_custom(1, glyph);

    while (1)
    {
        if (started == 1)
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
                    joystick_command('c');
                }
                else if (x > 1 - deadzone) //baixo
                {
                    joystick_command('b');
                }
                disp_update();
                _delay_ms(250);
            }
        }
        else
        {
            if (tic == 65535)
            {
                tic = 0;
            }
            else
            {
                tic++;
            }
        }
    }
}
