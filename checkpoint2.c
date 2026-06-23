// projeto 2 - checkpoint 2
// Compilador: MikroC PRO for PIC
// Nomes: 
// Felipe Maitan - 13748652 
// Eduardo Rezende - 15448693 
// Matheus Herberts - 15653174

// LCD 16x2 em modo 4 bits no PORTD
sbit LCD_RS at LATD4_bit;
sbit LCD_EN at LATD5_bit;
sbit LCD_D4 at LATD0_bit;
sbit LCD_D5 at LATD1_bit;
sbit LCD_D6 at LATD2_bit;
sbit LCD_D7 at LATD3_bit;

sbit LCD_RS_Direction at TRISD4_bit;
sbit LCD_EN_Direction at TRISD5_bit;
sbit LCD_D4_Direction at TRISD0_bit;
sbit LCD_D5_Direction at TRISD1_bit;
sbit LCD_D6_Direction at TRISD2_bit;
sbit LCD_D7_Direction at TRISD3_bit;

// Estados do programa
#define ESPERANDO   0
#define CONT_LONGA  1
#define CONT_CURTA  2
#define TERMINOU    3

// Variaveis usadas tambem dentro da interrupcao
volatile char segundos_longo = 0;
volatile char segundos_curto = 0;
volatile char cont_250ms = 0;
volatile char estado = ESPERANDO;
volatile bit pede_lcd;

// Recarrega o Timer0 para aproximadamente 1 segundo
void recarrega_timer0() {
    TMR0H = 0xC2;
    TMR0L = 0xF7;
}

// Recarrega o Timer1 para aproximadamente 250 ms
void recarrega_timer1() {
    TMR1H = 0x0B;
    TMR1L = 0xDC;
}

// Inicia a contagem longa
void iniciar_longo() {
    segundos_longo = 60;
    estado = CONT_LONGA;

    recarrega_timer0();
    TMR0ON_bit = 1;
    TMR1ON_bit = 0;

    pede_lcd = 1;
}

// Inicia a contagem curta
void iniciar_curto() {
    segundos_curto = 10;
    cont_250ms = 0;
    estado = CONT_CURTA;

    recarrega_timer1();
    TMR1ON_bit = 1;
    TMR0ON_bit = 0;

    pede_lcd = 1;
}

// Tratamento das interrupcoes
void interrupt() {

    // INT0: botao da contagem longa
    if (INT0IF_bit == 1) {
        INT0IF_bit = 0;
        iniciar_longo();
    }

    // INT1: botao da contagem curta
    if (INTCON3.INT1IF == 1) {
        INTCON3.INT1IF = 0;
        iniciar_curto();
    }

    // Timer0: base de 1 segundo
    if (TMR0IF_bit == 1) {
        TMR0IF_bit = 0;
        recarrega_timer0();

        if (estado == CONT_LONGA) {
            if (segundos_longo > 0) {
                segundos_longo--;
                pede_lcd = 1;
            }
            else {
                estado = TERMINOU;
                TMR0ON_bit = 0;
                pede_lcd = 1;
            }
        }
    }

    // Timer1: base de 250 ms
    if (TMR1IF_bit == 1) {
        TMR1IF_bit = 0;
        recarrega_timer1();

        if (estado == CONT_CURTA) {
            cont_250ms++;

            // 4 estouros de 250 ms formam 1 segundo
            if (cont_250ms >= 4) {
                cont_250ms = 0;

                if (segundos_curto > 0) {
                    segundos_curto--;
                    pede_lcd = 1;
                }
                else {
                    estado = TERMINOU;
                    TMR1ON_bit = 0;
                    pede_lcd = 1;
                }
            }
        }
    }
}

// Atualiza os textos do LCD
void desenha_lcd() {
    char valor;
    char dezena;
    char unidade;

    valor = 0;

    if (estado == CONT_LONGA) {
        valor = segundos_longo;
    }
    else if (estado == CONT_CURTA) {
        valor = segundos_curto;
    }

    dezena = (valor / 10) + '0';
    unidade = (valor % 10) + '0';

    if (estado == CONT_LONGA) {
        Lcd_Out(1, 1, "Timer: LONGO   ");
        Lcd_Out(2, 1, "Restam: 00s    ");
        Lcd_Chr(2, 9, dezena);
        Lcd_Chr(2, 10, unidade);
    }
    else if (estado == CONT_CURTA) {
        Lcd_Out(1, 1, "Timer: CURTO   ");
        Lcd_Out(2, 1, "Restam: 00s    ");
        Lcd_Chr(2, 9, dezena);
        Lcd_Chr(2, 10, unidade);
    }
    else if (estado == TERMINOU) {
        Lcd_Out(1, 1, "Finalizado!    ");
        Lcd_Out(2, 1, "               ");
    }
    else {
        Lcd_Out(1, 1, " Aguardando... ");
        Lcd_Out(2, 1, "               ");
    }
}

void main() {

    // Todas as entradas analogicas como digitais
    ADCON1 = 0x0F;
    CMCON = 0x07;

    // Botoes nos pinos RB0/INT0 e RB1/INT1
    TRISB0_bit = 1;
    TRISB1_bit = 1;

    // Timer0 desligado inicialmente, 16 bits, clock interno, prescaler 1:128
    T0CON = 0b00000110;

    // Timer1 desligado inicialmente, 16 bits, prescaler 1:8
    T1CON = 0b10110000;

    // Interrupcoes externas por borda de subida
    INTCON2.INTEDG0 = 1;
    INTCON2.INTEDG1 = 1;

    INT0IF_bit = 0;
    INT0IE_bit = 1;

    INTCON3.INT1IF = 0;
    INTCON3.INT1IE = 1;

    // Interrupcoes dos timers
    TMR0IF_bit = 0;
    TMR0IE_bit = 1;

    TMR1IF_bit = 0;
    TMR1IE_bit = 1;

    PEIE_bit = 1;
    GIE_bit = 1;

    // Inicializacao do LCD
    Delay_ms(200);
    Lcd_Init();
    Lcd_Cmd(_LCD_CURSOR_OFF);

    pede_lcd = 1;

    while (1) {
        if (pede_lcd == 1) {
            pede_lcd = 0;
            Delay_ms(10);
            desenha_lcd();
        }
    }
}
