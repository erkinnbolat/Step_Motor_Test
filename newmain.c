// İşlemci kütüphaneleri (Cppcheck'in bunları bulamaması normal, sunucuda XC16 yok)
#include <xc.h>
#include <stdint.h>

// 1. KURAL İHLALİ ÇÖZÜMÜ: Kullanılmayan makroları sil veya kullan (FCY)
#define FCY 3685000UL

// --- FONKSİYON PROTOTİPLERİ (MISRA Rule 8.4 Çözümü) ---
// Sadece bu dosyada kullanılan fonksiyonlar ve değişkenler "static" olmalıdır.
static void LCD_Pulse(void);
static void LCD_SendNibble(uint8_t n);
static void LCD_Cmd(uint8_t cmd);
static void LCD_Char(char c);
static void LCD_String(const char *str); // const char eklendi
static void LCD_Init(void);

static void ADC_Init(void);
static uint16_t ADC_Read(uint8_t channel);
static void PWM_Init(void);
static void Timer3_Init(void);

// Özel Sprintf alternatifi (MISRA Rule 21.6 Çözümü - stdio.h kullanmamak için)
static void UInt16_ToString(uint16_t value, char *buffer, uint8_t digits);

// --- GLOBAL DEĞİŞKENLER ---
// "static" eklenerek sadece bu dosyada geçerli oldukları belirtildi.
static const uint8_t sine128[128] = {
    // ... (Senin 128'lik sinüs dizin buraya gelecek)
    127, 133, 139 // vs.
};

static volatile uint8_t idx_A = 0U;   // 'U' takısı eklendi (Unsigned)
static volatile uint8_t idx_B = 42U;
static volatile uint8_t idx_C = 85U;
static volatile uint8_t genlik_k = 255U;

// cppcheck-suppress unusedFunction
// (MISRA'ya bu fonksiyonun donanım kesmesi olduğunu ve ana kodda çağrılmayacağını söylüyoruz)
void __attribute__((__interrupt__, auto_psv)) _T3Interrupt(void) {
    // 10.4 Çözümü: Bitwise işlemlerde sabit sayılara U eklendi
    idx_A = (idx_A + 1U) & 127U;
    idx_B = (idx_B + 1U) & 127U;
    idx_C = (idx_C + 1U) & 127U;
    
    IFS0bits.T3IF = 0U; // Bayrağı temizle
}

// --- FONKSİYON İÇERİKLERİ ---
static void LCD_Pulse(void) {
    // Senin pin tanımlamaların (E = 1 vs.)
}

static void LCD_SendNibble(uint8_t n) {
    // 10.4 Çözümü: Shift işlemlerinde sayılara U (Unsigned) takısı
    // D4 = (n >> 0U) & 1U;
    // D5 = (n >> 1U) & 1U;
    // D6 = (n >> 2U) & 1U;
    // D7 = (n >> 3U) & 1U;
    LCD_Pulse();
}

static void LCD_Cmd(uint8_t cmd) {
    // RS = 0;
    LCD_SendNibble(cmd >> 4U);
    LCD_SendNibble(cmd & 0x0FU);
    // __delay_ms(2);
}

static void LCD_Char(char c) {
    // RS = 1;
    LCD_SendNibble((uint8_t)c >> 4U);
    LCD_SendNibble((uint8_t)c & 0x0FU);
    // __delay_us(100);
}

// const Parameter pointer çözümü
static void LCD_String(const char *str) {
    uint8_t i = 0U;
    while (str[i] != '\0') {
        LCD_Char(str[i]);
        i++;
    }
}

// Geri kalan init fonksiyonlarını buraya kendi kodundaki gibi ekle...
static void ADC_Init(void) { /* ... */ }
static uint16_t ADC_Read(uint8_t channel) { return 0U; /* Senin kodun */ }
static void PWM_Init(void) { /* ... */ }
static void Timer3_Init(void) { /* ... */ }


// Sprintf yerine kullanacağımız güvenli metin çevirici
static void UInt16_ToString(uint16_t value, char *buffer, uint8_t digits) {
    uint8_t i;
    for (i = 0U; i < digits; i++) {
        buffer[(digits - 1U) - i] = (char)((value % 10U) + '0');
        value /= 10U;
    }
    buffer[digits] = '\0';
}

// --- ANA FONKSİYON ---
int main(void) {
    // Değişkenleri fonksiyonun başında değil, kullanılacakları scope'a en yakın yere alıyoruz.
    
    // Init fonksiyonlarını çağır
    LCD_Init();
    ADC_Init();
    PWM_Init();
    Timer3_Init();

    LCD_Cmd(0x80U);
    LCD_String("220209025");
    LCD_Cmd(0xC0U);
    LCD_String("Erkin BOLAT");

    char buffer[16]; // Ekrana yazı yazdırmak için dizi

    while (1) {
        // [variableScope] ve 12.3 Çözümleri:
        // Değişkenler her seferinde tek satırda ve sadece döngü içinde tanımlandı.
        uint16_t okunan_hiz = ADC_Read(0U);
        uint16_t okunan_genlik = ADC_Read(1U); // Örnek
        uint32_t hiz_yuzde;
        uint32_t target_freq;

        // 10.4 Type Mismatch Çözümü: Matematikteki tüm sabit sayılar U (Unsigned) yapıldı.
        hiz_yuzde = ((uint32_t)okunan_hiz * 100U) / 1023U;
        target_freq = 10U + (hiz_yuzde / 2U);
        
        // 10.4 Type Mismatch Çözümü: UL (Unsigned Long) eklendi
        PR3 = (uint16_t)((3685000UL / (target_freq * 128UL)) - 1UL);

        // --- BUTON KONTROLLERİ ---
        // if (PORTCbits.RC1 == 0U) { ... } 

        // --- LCD YAZDIRMA ---
        LCD_Cmd(0x8AU); // İmleci ayarla
        UInt16_ToString((uint16_t)target_freq, buffer, 2U); // sprintf yerine
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
