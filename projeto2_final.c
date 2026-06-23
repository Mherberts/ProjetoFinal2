// projeto 2 - entrega final
// Compilador: MikroC PRO for PIC
// Microcontrolador: PIC18F4550
// Clock: 8 MHz

// Definições dos pinos de controle e dados do LCD
sbit LCD_RS at LATD4_bit;
sbit LCD_EN at LATD5_bit;
sbit LCD_D4 at LATD0_bit;
sbit LCD_D5 at LATD1_bit;
sbit LCD_D6 at LATD2_bit;
sbit LCD_D7 at LATD3_bit;

// Definições da direção (entrada/saída) dos pinos do LCD
sbit LCD_RS_Direction at TRISD4_bit;
sbit LCD_EN_Direction at TRISD5_bit;
sbit LCD_D4_Direction at TRISD0_bit;
sbit LCD_D5_Direction at TRISD1_bit;
sbit LCD_D6_Direction at TRISD2_bit;
sbit LCD_D7_Direction at TRISD3_bit;

// Pino e direção do LED indicador
sbit LED at LATC0_bit;
sbit LED_Direction at TRISC0_bit;

// Constantes para os estados do sistema
#define ESPERANDO   0
#define CONT_LONGA  1
#define CONT_CURTA  2
#define TERMINOU    3

// Variáveis globais de controle
volatile char segundos_longo = 0; // Contador de segundos para a contagem longa
volatile char segundos_curto = 0; // Contador de segundos para a contagem curta
volatile char cont_250ms = 0;     // Auxiliar para contar 1 segundo no Timer1
volatile char estado = ESPERANDO; // Estado inicial do sistema
volatile bit pede_lcd;            // Flag que sinaliza quando atualizar a interface do LCD

unsigned int temperatura_x10 = 0; // Armazena a temperatura multiplicada por 10 (ex: 254 = 25.4 C)

// Recarrega o Timer0 para estourar a cada 1 segundo (com prescaler configurado no main)
void recarrega_timer0() {
    TMR0H = 0xC2;
    TMR0L = 0xF7;
}

// Recarrega o Timer1 para aproximadamente 250 ms
void recarrega_timer1() {
    TMR1H = 0x0B;
    TMR1L = 0xDC;
}

// Configura e inicia a contagem de 60 segundos usando Timer0
void iniciar_longo() {
    segundos_longo = 60;
    estado = CONT_LONGA;
    recarrega_timer0();
    TMR0ON_bit = 1;      // Liga Timer0
    TMR1ON_bit = 0;      // Desliga Timer1 para evitar conflito
    pede_lcd = 1;        // Solicita o redesenho do layout no LCD
}

// Configura e inicia a contagem de 10 segundos usando Timer1
void iniciar_curto() {
    segundos_curto = 10;
    cont_250ms = 0;
    estado = CONT_CURTA;
    recarrega_timer1();
    TMR1ON_bit = 1;      // Liga Timer1
    TMR0ON_bit = 0;      // Desliga Timer0
    pede_lcd = 1;        // Solicita o redesenho do layout no LCD
}

// Rotina principal de tratamento de interrupções
void interrupt() {
    // Interrupção externa 0 (botão aciona contagem longa)
    if (INT0IF_bit == 1) {
        INT0IF_bit = 0;  // Limpa a flag da interrupção
        iniciar_longo();
    }

    // Interrupção externa 1 (botão aciona contagem curta)
    if (INTCON3.INT1IF == 1) {
        INTCON3.INT1IF = 0; // Limpa a flag da interrupção
        iniciar_curto();
    }

    // Interrupção do Timer0 (Ocorre a cada 1 segundo)
    if (TMR0IF_bit == 1) {
        TMR0IF_bit = 0; // Limpa a flag
        recarrega_timer0();
        if (estado == CONT_LONGA) {
            if (segundos_longo > 0) segundos_longo--;
            else { estado = TERMINOU; TMR0ON_bit = 0; pede_lcd = 1; } // Para o timer ao zerar
        }
    }

    // Interrupção do Timer1 (Ocorre a cada 250 ms)
    if (TMR1IF_bit == 1) {
        TMR1IF_bit = 0; // Limpa a flag
        recarrega_timer1();
        if (estado == CONT_CURTA) {
            cont_250ms++;
            if (cont_250ms >= 4) { // Acumula 4x250ms para descontar 1 segundo
                cont_250ms = 0;
                if (segundos_curto > 0) segundos_curto--;
                else { estado = TERMINOU; TMR1ON_bit = 0; pede_lcd = 1; } // Para o timer ao zerar
            }
        }
    }
}

// Lê o canal analógico e converte para temperatura com uma casa decimal (escala x10)
unsigned int ler_temperatura_x10() {
    unsigned int leitura_adc = ADC_Get_Sample(0); // Lê do pino AN0
    unsigned long temp_x10 = ((unsigned long)leitura_adc * 1000) / 1023;
    return (unsigned int)temp_x10;
}

// Formata o valor bruto da temperatura e exibe no LCD
void mostra_temperatura(unsigned int valor_x10, char linha) {
    char dezena, unidade, decimal;
    unsigned int parte_inteira = valor_x10 / 10;

    if (parte_inteira > 99) parte_inteira = 99; // Limita o valor máximo de exibição
    decimal = (valor_x10 % 10) + '0';           // Extrai a casa decimal convertendo em ASCII
    dezena = (parte_inteira / 10) + '0';        // Extrai a dezena convertendo em ASCII
    unidade = (parte_inteira % 10) + '0';       // Extrai a unidade convertendo em ASCII

    // Imprime os caracteres nas colunas fixas
    Lcd_Chr(linha, 8, dezena);
    Lcd_Chr(linha, 9, unidade);
    Lcd_Chr(linha, 10, '.');
    Lcd_Chr(linha, 11, decimal);
}

// Atualiza o texto estático do LCD baseando-se no estado atual da máquina
void desenha_lcd() {
    Delay_ms(50);
    Lcd_Cmd(_LCD_CURSOR_OFF);

    if (estado == CONT_LONGA) {
        Lcd_Out(1, 1, " Temp: 00.0 C   ");
        Lcd_Out(2, 1, " Longa: 00s     ");
    }
    else if (estado == CONT_CURTA) {
        Lcd_Out(1, 1, " Temp: 00.0 C   ");
        Lcd_Out(2, 1, " Curta: 00s     ");
    }
    else if (estado == TERMINOU) {
        Lcd_Out(1, 1, " Finalizado!    ");
        Lcd_Chr(1, 16, ' '); 
        Lcd_Out(2, 1, "                ");
        LED = 0; // Garante que o LED apague ao finalizar
    }
    else {
        Lcd_Out(1, 1, " Aguardando...  ");
        Lcd_Out(2, 1, "                ");
        LED = 0; // Garante que o LED comece apagado
    }
}

// Sobrescreve as variáveis mutáveis (segundos e temperatura) no display
void atualiza_valores() {
    char dezena, unidade, valor;

    // Determina de onde tirar o tempo com base no estado
    if (estado == CONT_LONGA) valor = segundos_longo;
    else if (estado == CONT_CURTA) valor = segundos_curto;
    else return;

    dezena = (valor / 10) + '0';
    unidade = (valor % 10) + '0';

    // Imprime os segundos formatados
    Lcd_Chr(2, 9, dezena);
    Lcd_Chr(2, 10, unidade);

    // Lê a temperatura e atualiza a primeira linha do display
    temperatura_x10 = ler_temperatura_x10();
    mostra_temperatura(temperatura_x10, 1);

    // Acende o LED se a temperatura for maior que 50.0 C
    if (temperatura_x10 > 500) LED = 1; else LED = 0;
}

// Configurações iniciais e loop principal
void main() {
    OSCCON = 0b01110010; // Oscilador interno setado para 8 MHz
    CMCON = 0x07;        // Desativa os comparadores
    
    // Configura os pinos como entrada (1)
    TRISB0_bit = 1;      // INT0 (Botão longo)
    TRISB1_bit = 1;      // INT1 (Botão curto)
    TRISA0_bit = 1;      // AN0 (Sensor analógico)
    TRISA2_bit = 1;
    TRISA3_bit = 1;

    // Configura o pino do LED como saída (0) e inicia apagado
    LED_Direction = 0;
    LED = 0;

    T0CON = 0b00000110;  // Configura Timer0 (16-bits, prescaler 1:128)
    T1CON = 0b10110000;  // Configura Timer1 (16-bits, prescaler 1:8, R/W 16bits)

    // Configura e habilita as Interrupções
    INTCON2.INTEDG0 = 1; // Dispara na borda de subida (INT0)
    INTCON2.INTEDG1 = 1; // Dispara na borda de subida (INT1)
    INT0IF_bit = 0;      // Limpa flag INT0
    INT0IE_bit = 1;      // Habilita interrupção externa 0
    INTCON3.INT1IF = 0;  // Limpa flag INT1
    INTCON3.INT1IE = 1;  // Habilita interrupção externa 1
    TMR0IF_bit = 0;      // Limpa flag Timer0
    TMR0IE_bit = 1;      // Habilita interrupção do Timer0
    TMR1IF_bit = 0;      // Limpa flag Timer1
    TMR1IE_bit = 1;      // Habilita interrupção do Timer1

    PEIE_bit = 1;        // Habilita chaves de interrupções de periféricos
    GIE_bit = 1;         // Habilita chave geral de interrupções

    // Inicializa o módulo LCD
    Delay_ms(200);
    Lcd_Init();
    Lcd_Cmd(_LCD_CURSOR_OFF);

    // Inicializa o módulo ADC
    ADC_Init();
    ADCON1 = 0b00111010; // Define as portas analógicas (RA0 como Analógica, VREF interno)

    pede_lcd = 1; // Força a primeira impressão da interface na tela

    // Loop infinito do firmware
    while (1) {
        // Checa se o estado foi alterado e o layout precisa ser refeito
        if (pede_lcd == 1) {
            pede_lcd = 0;
            desenha_lcd();
        }

        // Atualiza a tela rotineiramente apenas durante a contagem
        if (estado == CONT_LONGA || estado == CONT_CURTA) {
            atualiza_valores();
            Delay_ms(200); // Taxa de atualização (evita piscar o LCD em excesso)
        }
    }
}
