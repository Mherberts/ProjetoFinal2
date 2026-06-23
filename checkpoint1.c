/*
 * SEL0433 - Projeto 2
 * Checkpoint 1
 * Eduardo Gondim Rezende - 15448693
 * Matheus Herberts Rios de Lima - 15653174
 * Felipe Bressanin Maitan - 13748652
 *
 * Microcontrolador: PIC18F4550
 * Clock: 8 MHz
 *
 * Fun��o:
 * - Escreve "HelloWrld" na linha 1 do LCD
 * - Mostra uma contagem de 0 a 9 na linha 2
 * - Incrementa a contagem a cada pressionamento do bot�o
 * - Trata bouncing usando debounce por software e flag auxiliar
 * - Detecta o bot�o por borda de subida
 */

/* Pinos do LCD no modo 4 bits */
sbit LCD_RS at RB4_bit;
sbit LCD_EN at RB5_bit;
sbit LCD_D4 at RB0_bit;
sbit LCD_D5 at RB1_bit;
sbit LCD_D6 at RB2_bit;
sbit LCD_D7 at RB3_bit;

/* Dire��o dos pinos usados pelo LCD */
sbit LCD_RS_Direction at TRISB4_bit;
sbit LCD_EN_Direction at TRISB5_bit;
sbit LCD_D4_Direction at TRISB0_bit;
sbit LCD_D5_Direction at TRISB1_bit;
sbit LCD_D6_Direction at TRISB2_bit;
sbit LCD_D7_Direction at TRISB3_bit;

/* Bot�o ligado no pino RD0 */
sbit botao at RD0_bit;

void main() {
    unsigned short contador = 0;
    unsigned short botao_ja_contado = 0;
    char digito = '0';

    /* Configura os pinos como digitais */
    ADCON1 = 0x0F;
    CMCON = 0x07;

    /* RD0 como entrada para o bot�o */
    TRISD0_bit = 1;

    /* Inicializa��o do LCD */
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);

    /* Texto fixo na primeira linha */
    Lcd_Out(1, 1, "HelloWrld");

    /* Valor inicial do contador na segunda linha */
    Lcd_Chr(2, 1, digito);

    while(1) {
        /*
         * Se o bot�o foi apertado e ainda n�o foi contado,
         * o programa espera um pouco para reduzir o bouncing.
         */
        if ((botao == 1) && (botao_ja_contado == 0)) {
            Delay_ms(20);

            /*
             * Depois do delay, verifica de novo se o bot�o
             * continua apertado. Se sim, o aperto � v�lido.
             */
            if (botao == 1) {
                botao_ja_contado = 1;

                contador++;

                if (contador > 9) {
                    contador = 0;
                }

                /*
                 * Converte o n�mero do contador para o caractere
                 * correspondente, para escrever no LCD.
                 */
                switch(contador) {
                    case 0: digito = '0'; break;
                    case 1: digito = '1'; break;
                    case 2: digito = '2'; break;
                    case 3: digito = '3'; break;
                    case 4: digito = '4'; break;
                    case 5: digito = '5'; break;
                    case 6: digito = '6'; break;
                    case 7: digito = '7'; break;
                    case 8: digito = '8'; break;
                    case 9: digito = '9'; break;
                }

                /*
                 * Atualiza somente o d�gito da contagem na linha 2.
                 */
                Lcd_Chr(2, 1, digito);
            }
        }

        /*
         * Quando o bot�o � solto, a flag � liberada.
         * Assim, o pr�ximo aperto poder� ser contado.
         */
        if (botao == 0) {
            botao_ja_contado = 0;
        }
    }
}
