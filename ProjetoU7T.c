#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#define IN_PIN 28    // GP28 (ADC2)
#define LED_PIN 13   // GP13 (Saída PWM de teste (LED RGB))
#define ADC_THRESHOLD 200 // Valor para ignorar o ruído do ADC

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

void outputOLED(){

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
    while (1) {
        loopLeitura();
    }
}
