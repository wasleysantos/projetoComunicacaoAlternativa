#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "oled/ssd1306.h"
#include "som/buzzer.h"
#include <string.h>
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include <math.h> // Para fabsf()

// Pinos I2C
const uint I2C_SDA = 14, I2C_SCL = 15;

// Pinos I2C para o MPU6050 (i2c0)
const uint I2C_SDA_MPU = 0, I2C_SCL_MPU = 1;

// Pinos periféricos
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define VRX_PIN 26
#define SW_PIN 22
#define BUZZER_PIN 10
#define BTN_A 5
#define BTN_B 6
#define CANAL_ADC_TEMPERATURA 4

// Definições do MPU6050
#define MPU6050_ADDR 0x68
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_TILT_THRESHOLD_FORWARD 8000 // Limiar para inclinação para frente (ajuste conforme necessário)

uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;
char temperatura_str[16];

// Variáveis de controle do menu
bool in_submenu = false;
int current_menu = 0, menu_index = 0;
bool sw_pressed = false, last_sw_state = false, selecting_option = false;
char word[100];
int word_index = 0;
char hora_atual[12] = "00:00:00";

// Menus (removida a opção "Letra")
const char *menus[][5] = {
    {"Voltar"},
    {"Ajuda", "Dor", "Nausea", "Confuso", "Posicao"},
    {"Agua", "Comer", "Banho", "Banheiro", "Andar"},
    {"Feliz", "Triste", "Medo", "Cansado", "Dormir"},
    {"Top", "Valeu", "Gosto", "Legal", "Feliz"}};

// Títulos dos menus (removida a opção "Alfabeto")
const char *menu_titles[] = {" TalkGo", "Alerta", "Pedido", "Humor", "Elogio"};

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 200 // Número de amostras que serão feitas do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f / 5.f) // Intervalos de volume do microfone.

// Função para desenhar texto escalonado no buffer do OLED
void ssd1306_draw_string_scaled(uint8_t *buffer, int x, int y, const char *text, int scale) {
    while (*text) {
        for (int dx = 0; dx < scale; dx++) {
            for (int dy = 0; dy < scale; dy++) {
                ssd1306_draw_char(buffer, x + dx, y + dy, *text);
            }
        }
        x += 6 * scale;
        text++;
    }
}

// Retorna a saudação dependendo da hora
const char *obter_saudacao(int hora) {
    if (hora >= 5 && hora < 12) {
        return "Bom dia";
    } else if (hora >= 12 && hora < 18) {
        return "Boa tarde";
    } else {
        return "Boa noite";
    }
}

// --- Funções do MPU6050 ---
void mpu6050_init(i2c_inst_t *i2c) {
    uint8_t buf[] = {MPU6050_REG_PWR_MGMT_1, 0x00};
    i2c_write_blocking(i2c, MPU6050_ADDR, buf, 2, false);
}

void mpu6050_read_accel(i2c_inst_t *i2c, int16_t accel[3]) {
    uint8_t buffer[6];
    uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
    i2c_write_blocking(i2c, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, MPU6050_ADDR, buffer, 6, false);
    accel[0] = (buffer[0] << 8) | buffer[1];
    accel[1] = (buffer[2] << 8) | buffer[3];
    accel[2] = (buffer[4] << 8) | buffer[5];
}

void obter_hora_formatada(char *buffer, size_t len) {
    datetime_t agora;
    rtc_get_datetime(&agora);
    snprintf(buffer, len, "%02d:%02d:%02d", agora.hour, agora.min, agora.sec);
}

// Função para ler temperatura do ADC
float ler_temperatura(void) {
    adc_select_input(CANAL_ADC_TEMPERATURA);
    uint16_t leitura = adc_read();
    float tensao = leitura * 3.3f / 4096;
    return 27 - (tensao - 0.706f) / 0.001721f; // Fórmula da RP2040
}

// Centraliza o texto no eixo X baseado no comprimento e escala
int text_centered_x(const char *text, int scale) {
    int len = strlen(text);
    int width = len * 6 * scale;
    return (ssd1306_width - width) / 2;
}

// Atualiza o display OLED com até 3 linhas
void atualizar_oled(const char *linha1, int escala1,
                    const char *linha2, int escala2,
                    const char *linha3, int escala3,
                    uint8_t *ssd, struct render_area *area) {
    memset(ssd, 0, ssd1306_buffer_length);

    if (linha1)
        ssd1306_draw_string_scaled(ssd, text_centered_x(linha1, escala1), 10, linha1, escala1);
    if (linha2)
        ssd1306_draw_string_scaled(ssd, text_centered_x(linha2, escala2), 30, linha2, escala2);
    if (linha3)
        ssd1306_draw_string_scaled(ssd, text_centered_x(linha3, escala3), 50, linha3, escala3);

    render_on_display(ssd, area);
}

// Atualiza LEDs conforme menu
void update_leds() {
    if ((current_menu == 1 && menu_index <= 5) || (current_menu == 2 && menu_index <= 5)) {
        gpio_put(LED_RED, true);
        gpio_put(LED_GREEN, false);
        gpio_put(LED_BLUE, false);
    } else if ((current_menu == 3 && menu_index <= 5) || (current_menu == 4 && menu_index <= 5)) {
        gpio_put(LED_GREEN, true);
        gpio_put(LED_RED, false);
        gpio_put(LED_BLUE, false);
    } else {
        gpio_put(LED_RED, false);
        gpio_put(LED_GREEN, false);
        gpio_put(LED_BLUE, true);
    }
}

// Sons para navegação
void play_tone_down() {
    tone(800, 50);
}

void play_tone_up() {
    tone(600, 50);
}

int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(VRX_PIN);

    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa I2C0 para o MPU6050
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(I2C_SDA_MPU, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_MPU, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_MPU);
    gpio_pull_up(I2C_SCL_MPU);

    mpu6050_init(i2c0);
    ssd1306_init();
    uint8_t dados[14];

    datetime_t t = {2025, 6, 30, 1, 20, 0, 0};
    rtc_init();
    rtc_set_datetime(&t);

    frame_area = (struct render_area){0, ssd1306_width - 1, 0, ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&frame_area);

    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    buzzer_init(BUZZER_PIN);

    datetime_t agora;
    rtc_get_datetime(&agora);
    snprintf(hora_atual, sizeof(hora_atual), "%02d:%02d:%02d", agora.hour, agora.min, agora.sec);
    char saudacao[16];
    strcpy(saudacao, obter_saudacao(agora.hour));

    atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);

    uint16_t last_vrx_value = 2048;
    absolute_time_t ultimo_update = get_absolute_time();

    // Variáveis para controle de debounce do MPU6050
    absolute_time_t last_mpu_nav_time = get_absolute_time();
    const uint64_t MPU_NAV_DEBOUNCE_US = 300000; // 300ms de debounce para MPU

    while (true) {
        adc_select_input(0);
        uint16_t vrx_value = adc_read();
        sw_pressed = !gpio_get(SW_PIN);

        // --- Leitura do MPU6050 ---
        int16_t accel[3];
        mpu6050_read_accel(i2c0, accel);
        
        // Verifica o tempo de debounce para navegação do MPU
        bool can_mpu_navigate = absolute_time_diff_us(last_mpu_nav_time, get_absolute_time()) >= MPU_NAV_DEBOUNCE_US;

        // Variáveis estáticas para armazenar leituras anteriores
        static int16_t last_accel_y = 0;
        static int16_t last_accel_x = 0; // Adicionada para o balanço para frente

        // Detecta pico de aceleração (chacoalhar)
        int16_t delta_y = accel[1] - last_accel_y;
        bool mpu_shake_right = (can_mpu_navigate && delta_y > 8000);  // Balanço para direita
        bool mpu_shake_left  = (can_mpu_navigate && delta_y < -8000); // Balanço para esquerda

        // Detecta inclinação para frente
        int16_t delta_x = accel[0] - last_accel_x;
        bool mpu_tilt_forward = (can_mpu_navigate && delta_x < -MPU6050_TILT_THRESHOLD_FORWARD); // Balanço para frente (negativo para inclinar para frente)

        bool moved_down = (vrx_value > 1000 && last_vrx_value <= 1000) || mpu_shake_right;
        bool moved_up   = (vrx_value < 3000 && last_vrx_value >= 3000) || mpu_shake_left;
        
        // Use mpu_tilt_forward to select an option, similar to sw_pressed
        bool mpu_select_option = mpu_tilt_forward;

        last_accel_y = accel[1];
        last_accel_x = accel[0]; // Atualiza a última leitura do eixo X

        if (gpio_get(BTN_B) == 0) {
            sleep_ms(200);
            if (gpio_get(BTN_B) == 0) {
                gpio_put(LED_GREEN, false);
                gpio_put(LED_BLUE, false);
                gpio_put(LED_RED, true);
                atualizar_oled("SOCORRO", 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                play_sos_alert(); // Assuming this function is defined elsewhere
                gpio_put(LED_RED, false);
                atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);
                in_submenu = false;
                menu_index = 0;
                current_menu = 0;
                selecting_option = false;
            }
        }

        if (!in_submenu) {
            if (moved_down) {
                current_menu = (current_menu + 1) % 5;
                play_tone_down();
                atualizar_oled(menu_titles[current_menu], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            } else if (moved_up) {
                current_menu = (current_menu - 1 + 5) % 5;
                play_tone_up();
                atualizar_oled(menu_titles[current_menu], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
            // Add mpu_select_option here to enter submenu
            if ((sw_pressed && !last_sw_state) || mpu_select_option) {
                in_submenu = true;
                menu_index = 0;
                sleep_ms(200);
                atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
        } else {
            if (moved_down) {
                menu_index = (menu_index + 1) % 5;
                play_tone_down();
                atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            } else if (moved_up) {
                menu_index = (menu_index - 1 + 5) % 5;
                play_tone_up();
                atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
            // Add mpu_select_option here to select an option within the submenu
            if ((sw_pressed && !last_sw_state) || mpu_select_option) {
                if (selecting_option) {
                    tone(current_menu == 1 ? 1000 : 400, current_menu == 1 ? 10000 : 500);
                    sleep_ms(500);
                } else if (current_menu == 1) {
                    selecting_option = true;
                    atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                    play_emergency_select_pattern(); // Assuming this function is defined elsewhere
                    sleep_ms(500);
                } else {
                    atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                    selecting_option = true;
                    play_normal_select_pattern(); // Assuming this function is defined elsewhere
                }
                sleep_ms(5000);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
            if (selecting_option && ((sw_pressed && !last_sw_state) || mpu_select_option)) { // Exit selection
                in_submenu = false;
                menu_index = 0;
                current_menu = 0;
                selecting_option = false;
                sleep_ms(500);
                atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
        }

        if (gpio_get(BTN_A) == 0) {
            sleep_ms(200); // Debounce button A
            if (gpio_get(BTN_A) == 0) {
                in_submenu = false;
                selecting_option = false;
                current_menu = 0;
                menu_index = 0;
                atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);
                last_mpu_nav_time = get_absolute_time(); // Reset MPU debounce timer
            }
        }

        // Atualiza relógio e saudação a cada 1 segundo
        if (absolute_time_diff_us(ultimo_update, get_absolute_time()) >= 1000000) {
            rtc_get_datetime(&agora);
            snprintf(hora_atual, sizeof(hora_atual), "%02d:%02d:%02d", agora.hour, agora.min, agora.sec);
            strcpy(saudacao, obter_saudacao(agora.hour));

            if (!in_submenu) {
                if (current_menu == 0) {
                    atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);
                } else {
                    atualizar_oled(menu_titles[current_menu], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
                }
            } else {
                atualizar_oled(menus[current_menu][menu_index], 2, NULL, 0, hora_atual, 2, ssd, &frame_area);
            }

            ultimo_update = get_absolute_time();
        }

        // --- DETECÇÃO DE QUEDA POR SOM ALTO ---
        adc_select_input(MIC_CHANNEL);
        int som_total = 0;
        for (int i = 0; i < SAMPLES; i++) {
            int leitura = adc_read();
            float volts = ADC_ADJUST(leitura);
            som_total += fabsf(volts);
        }

        float som_medio = som_total / (float)SAMPLES;

        if (som_medio > 0.6f) {
            atualizar_oled("QUEDA", 2, "SOCORRO", 2, hora_atual, 2, ssd, &frame_area);
            gpio_put(LED_BLUE, false);
            gpio_put(LED_RED, true);
            play_sos_alert(); // Assuming this function is defined elsewhere
            sleep_ms(5000);
            gpio_put(LED_RED, false);
            atualizar_oled("TalkGo", 3, saudacao, 2, hora_atual, 2, ssd, &frame_area);
        }

        update_leds();
        last_vrx_value = vrx_value;
        last_sw_state = sw_pressed;
        sleep_ms(100);
    }

    return 0;
}