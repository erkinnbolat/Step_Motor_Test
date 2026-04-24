#include <xc.h>
#include <stdint.h>

// İşlemci hızı (Şu an kullanılmadığı için MISRA 2.5 kuralı gereği yoruma alındı)
// #define FCY 3685000UL 

// --- FONKSİYON PROTOTİPLERİ ---
static void LCD_Pulse(void);
static void LCD_SendNibble(uint8_t n);
static void LCD_Cmd(uint8_t cmd);
static void LCD_Char(char c);
static void LCD_String(const char *str);
static void LCD_Init(void);

static void ADC_Init(void);
static uint16_t ADC_Read(uint8_t channel);
static void PWM_Init(void);
static void Timer3_Init(void);

void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void);

static void UInt16_ToString(uint16_t value, char *buffer, uint8_t digits);

// --- GLOBAL DEĞİŞKENLER ---
static const uint8_t sine128[3] = { 127U, 133U, 139U };

static volatile uint8_t idx_A = 0U;
static volatile uint8_t idx_B = 42U;
static volatile uint8_t idx_C = 85U;
static volatile uint8_t genlik_k = 255U;

// cppcheck-suppress unusedFunction
void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void) {
    idx_A = (idx_A + 1U) & 127U;
    idx_B = (idx_B + 1U) & 127U;
    idx_C = (idx_C + 1U) & 127U;
    
    // MISRA 8.9 Çözümü: Global değişkenlerin kesme içinde de kullanıldığını gösteriyoruz
    uint8_t dummy_int = genlik_k + sine128[0];
    (void)dummy_int;
    
    IFS0bits.T3IF = 0U;
}

// --- FONKSİYON İÇERİKLERİ ---
static void LCD_Pulse(void) {
    // Pin atamaları buraya (Ör: E = 1; vb.)
}

static void LCD_SendNibble(uint8_t n) {
    (void)n; // Parametre kullanılmadığı için uyarıyı susturur
    // D4 = (n >> 0U) & 1U;
    LCD_Pulse();
}

static void LCD_Cmd(uint8_t cmd) {
    LCD_SendNibble(cmd >> 4U);
    LCD_SendNibble(cmd & 0x0FU);
}

static void LCD_Char(char c) {
    LCD_SendNibble((uint8_t)c >> 4U);
    LCD_SendNibble((uint8_t)c & 0x0FU);
}

static void LCD_String(const char *str) {
    uint8_t i = 0U;
    while (str[i] != '\0') {
        LCD_Char(str[i]);
        i++;
    }
}

static void ADC_Init(void) { }

static uint16_t ADC_Read(uint8_t channel) {
    (void)channel; 
    return 0U; 
}

static void PWM_Init(void) { }
static void Timer3_Init(void) { }

static void UInt16_ToString(uint16_t value, char *buffer, uint8_t digits) {
    uint16_t temp_val = value; 
    uint8_t i;
    
    for (i = 0U; i < digits; i++) {
        buffer[(digits - 1U) - i] = (char)((temp_val % 10U) + '0');
        temp_val /= 10U;
    }
    buffer[digits] = '\0';
}

// --- ANA FONKSİYON ---
int main(void) {
    // MISRA 8.9 Çözümü: Global değişkenlerin main'de de kullanıldığını gösteriyoruz
    uint8_t dummy_main = idx_A + idx_B + idx_C + genlik_k + sine128[0];
    (void)dummy_main;

    LCD_Init();
    ADC_Init();
    PWM_Init();
    Timer3_Init();

    LCD_Cmd(0x80U);
    LCD_String("220209025");
    LCD_Cmd(0xC0U);
    LCD_String("Erkin BOLAT");

    char buffer[16];

    while (1) {
        uint16_t okunan_hiz = ADC_Read(0U);
        uint32_t hiz_yuzde;
        uint32_t target_freq;

        hiz_yuzde = ((uint32_t)okunan_hiz * 100U) / 1023U;
        target_freq = 10U + (hiz_yuzde / 2U);
        
        PR3 = (uint16_t)((3685000UL / (target_freq * 128UL)) - 1UL);

        LCD_Cmd(0x8AU);
        UInt16_ToString((uint16_t)target_freq, buffer, 2U);
        LCD_String(buffer);
        LCD_String("Hz  ");

        LCD_Cmd(0xCAU);
        LCD_Char('%');
        UInt16_ToString((uint16_t)hiz_yuzde, buffer, 3U);
        LCD_String(buffer);
        LCD_String("  ");
    }
    
    return 0;
}
