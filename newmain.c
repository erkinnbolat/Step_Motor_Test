// ================================================================
//  DSPIC33FJ32MC204 ? 3-FAZ ?NVERTÖR (OSC ÇALI?AN SADE SÜRÜM)
//  Ö?renci: Erkin BOLAT | No: 220209025
// ================================================================

#define FCY 3685000UL // 7.37 MHz / 2

#include <xc.h>
#include <libpic30.h>
#include <stdio.h>

// --- KONF?GÜRASYON B?TLER? ---
#pragma config FNOSC    = FRC       
#pragma config IESO     = OFF       
#pragma config FWDTEN   = OFF       
#pragma config JTAGEN   = OFF       
#pragma config ICS      = PGD1      

// --- LCD P?NLER? (RB0 - RB5) ---
#define RS      LATBbits.LATB0
#define E       LATBbits.LATB1
#define D4      LATBbits.LATB2
#define D5      LATBbits.LATB3
#define D6      LATBbits.LATB4
#define D7      LATBbits.LATB5

// --- BUTON VE LED P?NLER? ---
#define SAGA1       PORTCbits.RC1  // Pin 26
#define SOLA1       PORTCbits.RC2  // Pin 27
#define yesil_led   LATCbits.LATC3 // Pin 36 (Ye?il)
#define yesil_kir   LATCbits.LATC4 // Pin 37 (K?rm?z? - Kapal? kalacak)
#define yesil_mav   LATCbits.LATC5 // Pin 38 (Mavi)

// --- 128 NOKTALI S?NÜS TABLOSU ---
const unsigned char sine128[128] = {
    127,133,139,145,151,157,163,169,174,180,185,190,195,199,203,207,
    211,214,217,220,222,224,226,228,229,230,231,231,231,231,230,229,
    228,226,224,222,220,217,214,211,207,203,199,195,190,185,180,174,
    169,163,157,151,145,139,133,127,121,115,109,103, 97, 91, 85, 79,
     74, 68, 63, 58, 53, 49, 45, 41, 37, 34, 31, 28, 26, 24, 22, 20,
     19, 18, 17, 17, 17, 17, 18, 19, 20, 22, 24, 26, 28, 31, 34, 37,
     41, 45, 49, 53, 58, 63, 68, 74, 79, 85, 91, 97,103,109,115,121,
    127,133,139,145,151,157,163,169,174,180,185,190,195,199,203,207
};

volatile unsigned char idx_A = 0;   // Faz 1 (H1)
volatile unsigned char idx_B = 42;  // Faz 2 (H2)
volatile unsigned char idx_C = 85;  // Faz 3 (H3)

volatile unsigned char genlik_k = 255; 

// ================================================================
//  LCD FONKS?YONLARI
// ================================================================
void LCD_Pulse(void) { E = 1; __delay_us(5); E = 0; __delay_us(50); }
void LCD_SendNibble(unsigned char n) {
    D4 = (n >> 0) & 1; D5 = (n >> 1) & 1;
    D6 = (n >> 2) & 1; D7 = (n >> 3) & 1; LCD_Pulse();
}
void LCD_Cmd(unsigned char cmd) { RS = 0; LCD_SendNibble(cmd >> 4); LCD_SendNibble(cmd & 0x0F); __delay_ms(2); }
void LCD_Char(char c) { RS = 1; LCD_SendNibble((unsigned char)c >> 4); LCD_SendNibble((unsigned char)c & 0x0F); __delay_us(100); }
void LCD_String(char *str) { while (*str) LCD_Char(*str++); }
void LCD_Init(void) {
    __delay_ms(50); RS = 0; E = 0;
    LCD_SendNibble(0x03); __delay_ms(5);
    LCD_SendNibble(0x03); __delay_us(150);
    LCD_SendNibble(0x03); __delay_us(150);
    LCD_SendNibble(0x02); __delay_us(150);
    LCD_Cmd(0x28); LCD_Cmd(0x0C); LCD_Cmd(0x01); __delay_ms(5);
}

// ================================================================
//  ADC
// ================================================================
void ADC_Init(void) {
    AD1PCFGL = 0xFFBE; // SADECE AN0(RA0) ve AN6(RC0) Analog
    AD1CON1 = 0x00E0;  
    AD1CON2 = 0x0000;
    AD1CON3 = 0x1F02;
    AD1CON1bits.ADON = 1;
}

uint16_t ADC_Read(uint8_t channel) {
    AD1CHS0bits.CH0SA = channel;
    AD1CON1bits.SAMP = 1;
    uint16_t t = 0;
    while (!AD1CON1bits.DONE) { if(++t > 5000) break; }
    AD1CON1bits.DONE = 0;
    return ADC1BUF0;
}

// ================================================================
//  PWM (ÇALI?AN B?REB?R AYARLAR)
// ================================================================
void PWM_Init(void) {
    __builtin_write_OSCCONL(OSCCON & ~(1 << 6));
    RPOR4bits.RP8R = 18; // OC1 -> RB8 (Pin 44)
    RPOR4bits.RP9R = 19; // OC2 -> RB9 (Pin 1)  
    __builtin_write_OSCCONL(OSCCON | (1 << 6));

    T2CON = 0x0000; TMR2 = 0; PR2 = 255; 
    
    OC1CON = 0; OC1R = 127; OC1RS = 127; OC1CONbits.OCTSEL = 0; OC1CONbits.OCM = 6;
    OC2CON = 0; OC2R = 127; OC2RS = 127; OC2CONbits.OCTSEL = 0; OC2CONbits.OCM = 6;
    T2CONbits.TON = 1;

    P1TCONbits.PTEN = 0; P1TCONbits.PTCKPS = 0; P1TCONbits.PTMOD = 0;
    P1TPER = 255;
    PWM1CON1 = 0x0000;
    PWM1CON1bits.PEN3H = 1; 
    
    P1OVDCON = 0x3F00; 
    P1FLTACON = 0x0000; 
    P1DC3 = 127 << 1;
    P1TCONbits.PTEN = 1;
}

// ================================================================
//  S?NÜS KESMES?
// ================================================================
void Timer3_Init(void) {
    T3CONbits.TON = 0; T3CONbits.TCKPS = 0; PR3 = 2000;
    IPC2bits.T3IP = 5; IFS0bits.T3IF = 0; IEC0bits.T3IE = 1; T3CONbits.TON = 1;
}

void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void) {
    unsigned int duty;
    
    duty = ((unsigned int)sine128[idx_A] * genlik_k) / 255u; OC1RS = duty;       
    duty = ((unsigned int)sine128[idx_B] * genlik_k) / 255u; OC2RS = duty;       
    duty = ((unsigned int)sine128[idx_C] * genlik_k) / 255u; P1DC3 = duty << 1;  

    idx_A = (idx_A + 1) & 127;
    idx_B = (idx_B + 1) & 127;
    idx_C = (idx_C + 1) & 127;

    IFS0bits.T3IF = 0;
}

// ================================================================
//  ANA DÖNGÜ
// ================================================================
int main(void) {
    
    AD1PCFGL = 0xFFBE; 
    
    TRISA = 0x0001; LATA = 0x0000; 
    TRISB = 0x0000; LATB = 0x0000; 
    TRISC = 0x0007; LATC = 0x0000; 

    ADC_Init(); LCD_Init(); PWM_Init(); Timer3_Init();

    LCD_Cmd(0x80); LCD_String("220209025");
    LCD_Cmd(0xC0); LCD_String("Erkin BOLAT");

    uint16_t okunan_hiz, okunan_genlik;
    uint32_t hiz_yuzde, target_freq;
    char buffer[16];

    while (1) {
        // --- 1. SENSÖR OKUMALARI VE HESAPLAR ---
        okunan_genlik = ADC_Read(6); 
        genlik_k = (unsigned char)(okunan_genlik >> 2); 

        okunan_hiz = ADC_Read(0); 
        hiz_yuzde = ((uint32_t)okunan_hiz * 100) / 1023; 
        
        target_freq = 10 + (hiz_yuzde / 2); 
        PR3 = 3685000 / (target_freq * 128) - 1; 

        // --- 2. SADECE BUTONLARA BA?LI KUSURSUZ LED MANTI?I ---
        yesil_kir = 0; // K?rm?z? LED her zaman kapal?

        if (SAGA1 == 0) {
            yesil_led = 1; // Sa?a bas?nca SADECE Ye?il LED yanar
        } else {
            yesil_led = 0;
        }

        if (SOLA1 == 0) {
            yesil_mav = 1; // Sola bas?nca SADECE Mavi LED yanar
        } else {
            yesil_mav = 0;
        }

        // --- 3. LCD GÜNCELLEMES? ---
        LCD_Cmd(0x8A); sprintf(buffer, "%02uHz  ", (uint16_t)target_freq); LCD_String(buffer);
        LCD_Cmd(0xCA); sprintf(buffer, "%%%03u  ", (uint16_t)hiz_yuzde); LCD_String(buffer);

        __delay_ms(50); 
    }
    return 0;
}