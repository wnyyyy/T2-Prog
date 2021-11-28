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

#define LIMITE_TEMPO 30
#define LIMITE_TENTATIVAS 10
#define TIMER_CLK F_CPU / 256
#define LED_1_V PC5
#define LED_2_V PC4
#define LED_3_V PC3
#define LED_4_V PC2
#define LED_1_A PD7
#define LED_2_A PD6
#define LED_3_A PD5
#define LED_4_A PD4

char pw_list[4] = {'+', 'o', '$', '*'}; // lista de caracteres q compõe a senha
float deadzone = 0.220f; // zona q analógico nao responde 
uint16_t tic = 0; // contador de tics q incrementa enquanto o jogo nao está executando, para gerar um número pseudorandom
int sel_index; // index do caractere do input selecionado
int started = 0; // 1/0 jogo iniciado
int gameover; // 1 - derrota, 2 - vitoria
char *password; 
int tentativas;
char *input;
int input_controller[4] = {0, 0, 0, 0}; //guarda qual simbolo está no input
int contador; // timer para cada tentativa e também para reiniciar o jogo dps de game over

uint8_t glyph[] = {0b00010000, 0b00100100, 0b11100000, 0b00100100, 0b00010000};

void disp_update_tentativas();
void disp_show_password();
void disp_update();
void disp_update_input();
void disp_update_selector();
void disp_update_timer();
void joystick_command(char command);
void generate_password();
void enter_attempt();
void score_update(int acertos, int simbolos);
void disp_vitoria();
void disp_derrota();
void led_vitoria();
void setup_game();

// controlador para o joystick
void joystick_command(char command)
{
    switch (command)
    {
    // move input para esquerda
    case 'e':
        if (sel_index > 0)
            sel_index--;
        else
            sel_index = 3;
        break;
    // move input para direita
    case 'd':
        if (sel_index < 3)
            sel_index++;
        else
            sel_index = 0;
        break;

    //muda caractere selecionado do input para o proximo caractere
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

    //muda caractere selecionado do input para o caractere anterior
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

//seta variaveis necessárias para o jogo
void setup_game()
{
    tentativas = LIMITE_TENTATIVAS;
    contador = LIMITE_TEMPO;
    gameover = 0;
    sel_index = 0;
    for (int i = 0; i < 4; i++)
    {
        input_controller[i] = 0;
    }
    char first_char = pw_list[0];
    for (int i = 0; i < 4; i++)
    {
        input[i] = first_char;
    }
    disp_update();
}

//gera uma senha nova e prepara o jogo
void restart_game()
{
    started = 1;
    generate_password();
    setup_game();
}

//mostra tela de vitória
void disp_vitoria()
{
    nokia_lcd_set_cursor(16, 18);
    nokia_lcd_write_string("Vitoria!!", 1);
    nokia_lcd_render();
    led_vitoria();
}

//mostra tela de game over
void disp_derrota()
{
    nokia_lcd_set_cursor(6, 15);
    nokia_lcd_write_string("Game", 2);
    nokia_lcd_set_cursor(20, 32);
    nokia_lcd_write_string("Over!!", 2);
    nokia_lcd_render();
    led_derrota();
}

//aplica uma tentativa
void enter_attempt()
{
    contador = LIMITE_TEMPO; //reinicia timer de cada tentativa
    int acertos = 0;
    int simbolos = 0;
    int qtd_senha[4] = {0,0,0,0};
    int qtd_input[4] = {0,0,0,0};    

    for (int i = 0; i < 4; i++)
    {
        //conta quantos símbolos a partir do index de {'+', 'o', '$', '*'} estão presentes no input e na senha
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
        //conta um eventual acerto
        if (input[i] == password[i])
        {
            acertos++;
        }
    }

    //simbolos em comum é o menor valor de cada index para input e senha
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

    //desconsiderar os simbolos corretos q estao em posiçao correta, para facilitar
    simbolos = simbolos - acertos;

    print("\n");
    print("\n");
    print("acertos: ");
    printint(acertos);
    print("\n");
    print("simbolos corretos: ");
    printint(simbolos);

    score_update(acertos, simbolos);
}

// acende leds de acordo com acertos e posições, seta variaveis necessárias para caso o jogo termine em derrota por tentativa ou vitória
void score_update(int acertos, int simbolos)
{
    PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));

    switch (acertos)
    {
    case 1:
        PORTC |= (1 << LED_1_V);
        break;
    case 2:
        PORTC |= ((1 << LED_1_V) | (1 << LED_2_V));
        break;
    case 3:
        PORTC |= ((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V));
        break;
    case 4:
        PORTC |= ((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
        gameover = 2;
        started = 0;
        break;
    
    default:
        break;
    }

    switch (simbolos)
    {
    case 1:
        PORTD |= (1 << LED_1_A);
        break;
    case 2:
        PORTD |= (1 << LED_1_A | (1 << LED_2_A));
        break;
    case 3:
        PORTD |= (1 << LED_1_A | (1 << LED_2_A) | (1 << LED_3_A));
        break;
    case 4:
        PORTD |= (1 << LED_1_A | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));
        break;
    
    default:
        break;
    }

    tentativas--;
    if (tentativas == 0 && gameover == 0)
    {
        gameover = 1;
        started = 0;
    }

    if (started == 0)
    {
        //incrementa o seed para evitar senha duplicada caso reinicie o jogo muito rápido
        if (tic == 65535)
        {
            tic = 0;
        }
        else
        {
            tic++;
        }
    }

    disp_update();
}

//atualiza o display
void disp_update()
{
    //pausa interrupções para evitar bugs visuais
    cli();

    nokia_lcd_clear();
    disp_update_tentativas();

    // se o jogo terminou, mandar tela de vitória ou derrota
    if (gameover > 0)
    {       
        if (gameover == 2)
            disp_vitoria();
        else
        {
            //seta contador para reiniciar depois de alguns segundos em caso de derrota
            contador = 0;
            disp_derrota();
        }            
    }
    else
    {
        //caso ainda tenha jogo, atualizar oq for necessário
        disp_update_selector();
        disp_update_input();
        disp_update_timer();
        nokia_lcd_render();
    }

    //retoma interrupçoes
    sei();
}

void led_derrota()
{
    PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));

    PORTC |= (1 << LED_4_V);
    PORTD |= (1 << LED_1_A);
    _delay_ms(100);
    PORTC |= (1 << LED_3_V);
    PORTD |= (1 << LED_2_A);
    _delay_ms(100);
    PORTC |= (1 << LED_2_V);
    PORTD |= (1 << LED_3_A);
    _delay_ms(100);
    PORTC |= (1 << LED_1_V);
    PORTD |= (1 << LED_4_A);
    _delay_ms(100);
    PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));
    _delay_ms(200);
}

void led_vitoria()
{
    PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));

    PORTC |= (1 << LED_1_V);
    PORTD |= (1 << LED_4_A);
    _delay_ms(100);
    PORTC |= (1 << LED_2_V);
    PORTD |= (1 << LED_3_A);
    _delay_ms(100);
    PORTC |= (1 << LED_3_V);
    PORTD |= (1 << LED_2_A);
    _delay_ms(100);
    PORTC |= (1 << LED_4_V);
    PORTD |= (1 << LED_1_A);
    _delay_ms(100);
    PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));
    _delay_ms(400);

    for (int i = 0; i < 3; i++)
    {
        PORTC |= ((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
        PORTD |= ((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));
        _delay_ms(400);
        PORTC &= ~((1 << LED_1_V) | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
        PORTD &= ~((1 << LED_1_A) | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));
        _delay_ms(400);
    }
}

void disp_update_timer()
{
    nokia_lcd_set_cursor(12, 40);
    nokia_lcd_write_string("Tempo:", 1);
    char strTempo[2];
    nokia_lcd_set_cursor(50, 40);
    sprintf(strTempo, "%d", contador);
    nokia_lcd_write_string(strTempo, 1);
}

void disp_update_selector()
{
    nokia_lcd_set_cursor(sel_index * 20 + 7, 30);
    nokia_lcd_write_char('^', 2);
}

void disp_update_tentativas()
{
    nokia_lcd_set_cursor(2, 2);
    nokia_lcd_write_string("Tentativas:", 1);
    nokia_lcd_drawline(0, 11, 84, 11);
    char strTentativas[2];
    nokia_lcd_set_cursor(68, 2);
    sprintf(strTentativas, "%d", tentativas);
    nokia_lcd_write_string(strTentativas, 1);
}

void disp_update_input()
{
    for (int i = 0; i < 4; i++)
    {
        nokia_lcd_set_cursor(i * 20 + 7, 14);
        nokia_lcd_write_char(input[i], 2);
    }
}

//gera uma senha com base num seed
void generate_password()
{
    print("\n\nseed: ");
    printint(tic);

    srandom(tic);
    for (int i = 0; i < 4; i++)
    {
        password[i] = pw_list[random() % 4];
    }

    print("\npassword: ");
    print(password);
}

//tratamento para o botão
ISR(INT0_vect)
{
    //se o jogo não foi iniciado ainda, o botão reinicia (ou começa) o jogo
    if (started == 0)
    {
        restart_game();
    }
    else
    {
    //caso contrário, o botão manda uma tentativa
        enter_attempt(); 
    }
    _delay_ms(25);
}

//tratamento para o contador
ISR(TIMER1_OVF_vect){
    // se o jogo já iniciou, conta o tempo restante para a tentativa
    if (started == 1)
    {
        contador--;
        TCNT1  = 65536 - TIMER_CLK;
        //se chegar em 0, o jogo acaba por limite de tempo
        if (contador == 0)
        {
            gameover = 1;
        }
        disp_update();
    }
    //se o jogo terminou em derrota, o contador serve para reiniciar o jogo após alguns segundos
    else
    {
        if (gameover == 1)
        {
            contador++;
            if (contador == 3)
            {
                restart_game();
            }
        }
    }
}

int main(void)
{
    cli();
    TCCR2A = 0x00;
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);

	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 65536 - TIMER_CLK;
	TCCR1B |= (1 << CS12);
	TIMSK1 |= (1 << TOIE1);

    USART_Init();
    adc_init();

    DDRD &= ~(1 << PD2);
    DDRC |= (1 << LED_1_V | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    DDRD |= (1 << LED_1_A | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));

    PORTD = 1 << PD2;
    EICRA = (1 << ISC01) | (1 << ISC00);
    EIMSK |= (1 << INT0);

    PORTC &= ~(1 << LED_1_V | (1 << LED_2_V) | (1 << LED_3_V) | (1 << LED_4_V));
    PORTD &= ~(1 << LED_1_A | (1 << LED_2_A) | (1 << LED_3_A) | (1 << LED_4_A));

    sei();

    password = malloc(4);
    input = malloc(4);
    
    setup_game();
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
                _delay_ms(400);
            }
        }
        else
        {
            //conta tics enquanto o jogo iniciou para gerar uma senha pseudoaleatória
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
