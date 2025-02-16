#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "ssd1306_i2c.h"

#define count_of(arr) (sizeof(arr) / sizeof((arr)[0])) 

#define I2C_SDA_OLED 14 // GP14 (SDA do OLED)
#define I2C_SCL_OLED 15 // GP15 (SCL do OLED)

#define IN_PIN 28    // GP28 (ADC2)
#define LED_PIN 13   // GP13 (Saída PWM de teste (LED RGB))
#define ADC_THRESHOLD 200 // Valor para ignorar o ruído do ADC

volatile static uint16_t valorA = 5;
volatile static uint16_t valorB = 0;

typedef struct {
    char **text;
    size_t lines;
} OLEDText; // Criando uma nova estrutura para os textos do OLED

OLEDText outputOLED(uint16_t botaoA, uint16_t botaoB) { // Função que retorna um texto para ser exibido no OLED
    if(botaoA == 0 && botaoB == 5){
        static char *text[] = {
            "Volume Buzzer:",
            "- - - - - ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText;
    } else if(botaoA == 1 && botaoB == 4){
        static char *text[] = {
            "Volume Buzzer:",
            "X - - - - ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText;
    } else if(botaoA == 2 && botaoB == 3){
        static char *text[] = {
            "Volume Buzzer:",
            "X X - - - ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText;
    } else if(botaoA == 3 && botaoB == 2){
        static char *text[] = {
            "Volume Buzzer:",
            "X X X - - ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText; 
    } else if(botaoA == 4 && botaoB == 1){
        static char *text[] = {
            "Volume Buzzer:",
            "X X X X - ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText;
    } else if(botaoA == 5 && botaoB == 0){
        static char *text[] = {
            "Volume Buzzer:",
            "X X X X X ",
            "AUMENTAR A",  
            "DIMINUIR B",
        };
        OLEDText oledText = { text, sizeof(text) / sizeof(text[0]) };
        return oledText;
    }
}

void updateOLED(){
    OLEDText oledText = outputOLED(valorA, valorB);
    char **text = oledText.text;
    size_t lines = oledText.lines;
}

void setupI2C() { 
    // Fazendo a configuração do I2C para o OLED (baseado no código de exemplo do Github)
    i2c_init(i2c1, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_OLED, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_OLED, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_OLED);
    gpio_pull_up(I2C_SCL_OLED);

    SSD1306_init();
    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    restart:

    SSD1306_scroll(true);
    sleep_ms(5000);
    SSD1306_scroll(false);

    OLEDText oledText = outputOLED(valorA, valorB);
    char **text = oledText.text;
    size_t lines = oledText.lines;

    int y = 0;
    for (size_t i = 0; i < lines; i++) {
        WriteString(buf, 5, y, text[i]);
        y += 8;
    }
    render(buf, &frame_area);
}

void setup() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(IN_PIN);  // Inicializa o ADC no GP28
    adc_select_input(2);    // Seleciona o ADC2 para o GP28
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM); // Configura o pino do LED RGB (de teste) para PWM
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    pwm_set_wrap(slice_num, 255);  
    pwm_set_enabled(slice_num, true);
}

void outputMatriz(){

}

void pwmBuzzer(){
    
}

void loopLeitura() {
    uint16_t val = adc_read(); // Lê o valor do ADC

    if (val >= ADC_THRESHOLD) { // Se o valor do ADC for maior que 200, o sistema ativa. Isso é para evitar que o ruído presente no ADC interfira no sistema
        val = val > 4000 ? 4000 : val; // Limita o valor do ADC a 4000
        val = (val - ADC_THRESHOLD) * (4095 - 1) / (4000 - ADC_THRESHOLD) + 1; // Mapeia 200-4000 para 1-4095
        pwm_set_gpio_level(LED_PIN, val / 16); // Divide o valor do ADC por 16 para caber na resolução do PWM (0-255)
    } else {
        pwm_set_gpio_level(LED_PIN, 0);
        val = 0;  // Qualquer valor abaixo de 200 é tratado como 0
    }
    sleep_ms(100);
}

int main() {
    setup();
    setupI2C();
    while (1) {
        loopLeitura();
    }
}
