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
#include "hardware/timer.h" 
#include "hardware/clocks.h"

#define count_of(arr) (sizeof(arr) / sizeof((arr)[0])) 

#define I2C_SDA_OLED 14 // GP14 (SDA do OLED)
#define I2C_SCL_OLED 15 // GP15 (SCL do OLED)

#define BUZZER_1 10 // Buzzer 1 (GP10)
#define BUZZER_2 21 // Buzzer 2 (GP21)

#define BUTTON_A 5 // Botão A (GP5)
#define BUTTON_B 6 // Botão B (GP6)

// Definições para realizar debouncing por temporizadores
static uint32_t ultimoTempoA = 0;
static uint32_t ultimoTempoB = 0;
const uint32_t debounceDelay = 200; // 200ms de debounce

#define IN_PIN 28    // GP28 (ADC2)
#define LED_PIN 13   // GP13 (Saída PWM de teste (LED RGB))
#define ADC_THRESHOLD 60 // Valor para ignorar o ruído do ADC

volatile static uint16_t valorA = 1;
volatile static uint16_t valorB = 5;
volatile static float multiplicadorVolume = 1;

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

void updateOLED(uint16_t botaoA, uint16_t botaoB) {
    
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

    OLEDText oledText = outputOLED(botaoA, botaoB);
    char **text = oledText.text;
    size_t lines = oledText.lines;

    int y = 0;
    for (size_t i = 0; i < lines; i++) {
        WriteString(buf, 5, y, text[i]);
        y += 8;
    }
    render(buf, &frame_area);
}

static void apertarBotao(uint gpio, uint32_t events) {
    // Primeiramente, obtém o valor de quando botão foi apertado
    uint32_t tempoAtual = to_ms_since_boot(get_absolute_time());
    // Para depois fazer um if e verificar se esse tempo está dentro do limite do debounce
    // (200ms desde a última vez que o botão foi apertado)

    // Interrupção (callback) para quando os botões forem apertados
    // Quanto mais A: Maior o volume
    // Quanto mais B: Menor o volume
    // A soma de A e B sempre deve ser 5
    if (gpio == BUTTON_A && valorA < 5 && (tempoAtual - ultimoTempoA) > debounceDelay) {
        ultimoTempoA = tempoAtual; // Atualiza o tempo do último botão A
        valorA++;
        valorB--;
    } else if (gpio == BUTTON_B && valorB < 5 && (tempoAtual - ultimoTempoB) > debounceDelay) {
        ultimoTempoB = tempoAtual; // Atualiza o tempo do último botão B
        valorB++;
        valorA--;
    }
    multiplicadorVolume = valorA * 0.2; // Calcula o multiplicador do volume baseado no valor de A
    updateOLED(valorA, valorB);
} 

void setupBuzzer() {
    // Configuração do buzzer
    gpio_init(BUZZER_1);
    gpio_set_dir(BUZZER_1, GPIO_OUT);
    gpio_put(BUZZER_1, 0);
    gpio_init(BUZZER_2);
    gpio_set_dir(BUZZER_2, GPIO_OUT);
    gpio_put(BUZZER_2, 0);

    // Configuração do PWM do buzzer
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_2, GPIO_FUNC_PWM);
    uint slice1 = pwm_gpio_to_slice_num(BUZZER_1);
    uint slice2 = pwm_gpio_to_slice_num(BUZZER_2);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice1, &config, true);
    pwm_init(slice2, &config, true);
    pwm_set_gpio_level(BUZZER_1, 0);
    pwm_set_gpio_level(BUZZER_2, 0);    
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
    sleep_ms(1500); // Diminui esse tempo para inicializar o OLED mais rapidamente
    SSD1306_scroll(false);

    OLEDText oledText = outputOLED(5, 0);
    valorA = 5;
    valorB = 0;
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

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &apertarBotao);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
}

void outputMatriz(){

}

void pwmBuzzer() {
    uint slice1 = pwm_gpio_to_slice_num(BUZZER_1);
    uint slice2 = pwm_gpio_to_slice_num(BUZZER_2);
    
    uint16_t val = adc_read(); // Lê o ADC
    if (val < ADC_THRESHOLD) val = 0; // Ignora ruídos baixos

    // Mapeia o ADC (0-4095) para uma frequência entre 200 Hz e 2000 Hz
    uint freq = 200 + ((val * 1800) / 4095);

    // Calcula os valores de PWM para configurar a frequência
    uint32_t clock_sys = clock_get_hz(clk_sys);
    uint32_t wrap = clock_sys / freq;

    pwm_set_wrap(slice1, wrap);
    pwm_set_wrap(slice2, wrap);

    // Define o nível PWM baseado no volume
    uint16_t volume = (val * multiplicadorVolume / 4);
    
    pwm_set_gpio_level(BUZZER_1, volume);
    pwm_set_gpio_level(BUZZER_2, volume);
}


void loopLeitura() {
    uint16_t val = adc_read(); // Lê o valor do ADC

    if (val >= ADC_THRESHOLD) { // Se o valor do ADC for maior que 800, o sistema ativa. Isso é para evitar que o ruído presente no ADC interfira no sistema
        val = val > 4000 ? 4000 : val; // Limita o valor do ADC a 4000
        val = (val - ADC_THRESHOLD) * (4095 - 1) / (4000 - ADC_THRESHOLD) + 1; // Mapeia 800-4000 para 1-4095
        pwm_set_gpio_level(LED_PIN, val / 16); // Divide o valor do ADC por 16 para caber na resolução do PWM (0-255)
    } else {
        pwm_set_gpio_level(LED_PIN, 0);
        val = 0;  // Qualquer valor abaixo de 800 é tratado como 0
    }
    pwmBuzzer(val); // Atualiza o volume do buzzer
    sleep_ms(100);
}

int main() {
    setup();
    setupBuzzer();
    setupI2C();
    while (1) {
        loopLeitura();
        printf("A: %d, B: %d, Volume: %.2f%%\n", valorA, valorB, multiplicadorVolume * 100);
    }
}
