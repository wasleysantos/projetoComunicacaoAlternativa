/*
Aluno: Wasley dos Santos
Projeto: TalkGo - Comunicação Alternativa

Público-Alvo:
Idosos com Dificuldades Motoras ou na fala, Paralisia Cerebral, Esclerose Múltipla, 
Acidente Vascular Cerebral (AVC), Doenças Neuromusculares.

Resumo:
O objetivo deste projeto é facilitar a comunicação de pessoas com deficiência 
motora e/ou na fala, através de um sistema acessível que permite que o 
usuário escolha sua fala utilizando um joystick. A mensagem gerada é então 
apresentada de forma compreensível para quem a recebe através do LCD. 
Muitas vezes, a comunicação dessas pessoas é dificultada pelas condições 
de saúde e pela falta de soluções acessíveis. Com isso, meu objetivo é 
oferecer uma alternativa eficaz, especialmente considerando que os 
sistemas existentes são geralmente caros e de difícil acesso. Quero 
criar uma ferramenta inclusiva e de fácil utilização para melhorar a 
interação e a qualidade de vida dessas pessoas.

Instruções de uso:
Navegação no Menu: Utilize o joystick para mover para baixo ou cima e 
selecionar uma categoria (Emergência, Necessidades, Emoções, etc.).

Seleção de Opções: Pressione o botão central do joystick para entrar 
no submenu e escolha uma frase ou ação desejada.

Exibição da Mensagem: A opção selecionada será exibida no display LCD 
para facilitar a comunicação.

Modo Alfabeto: No menu "Alfabeto", navegue entre as letras com o 
joystick e confirme com botão A quando a palavra ja estiver formada.

Sinal de Emergência: Pressione o botão B para exibir “SOCORRO” no 
display e ativar um alerta sonoro e visual. 

Ultima atualização: 14/02/2025 08:23
*/

#include <stdio.h>  
#include "pico/stdlib.h"  
#include "hardware/adc.h" 
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <string.h>

// Definição dos pinos para comunicação I2C
const uint I2C_SDA = 14, I2C_SCL = 15;

// Definição dos pinos para LEDs e outros periféricos
#define LED_RED 13  
#define LED_GREEN 11  
#define LED_BLUE 12 
#define VRX_PIN 26  
#define VRY_PIN 27  
#define SW_PIN 22  
#define BUZZER_PIN 10  
#define BTN_A 5
#define BTN_B 6
#define CANAL_ADC_TEMPERATURA 4  
#define ALPHABET_SIZE (sizeof(alphabet) / sizeof(alphabet[0])) 

// Variáveis de controle do menu
bool in_submenu = false;  // Indica se estamos no submenu  
int current_menu = 0, menu_index = 0, last_vry_value = 0;  
bool sw_pressed = false, last_sw_state = false, menu_updated = false, selecting_option = false;  
char word[100];  // Buffer para armazenar a palavra montada
int word_index = 0; // Índice da palavra

// Definição das opções de menu
const char *menus[][5] = {  
    {"Voltar"},
    {"Ajuda", "Muita dor", "Enjoado", "Desorientado", "Mudar posicao"},
    {"Agua", "Comer", "Ao Banheiro", "Tomar banho", "Passear"},  
    {"Feliz", "Triste", "Com medo", "Cansado", "Dormir"},   
    {"Incrivel", "Obrigado", "Adoro voce", "Otimo trabalho", "Me faz feliz"},
    {"Selecionar Letra"}  
};  

// Definição dos títulos do menu principal
const char *menu_titles[] = {" TalkGo ", "Emergencia", "Necessidades", "Emocoes", "Elogios", "Alfabeto"}; 

// Alfabeto
const char *alphabet[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", 
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", 
    "U", "V", "W", "X", "Y", "Z"
};

// Função para desenhar uma string no display SSD1306 com escala
void ssd1306_draw_string_scaled(uint8_t *buffer, int x, int y, const char *text, int scale) {
    while (*text) {
        for (int dx = 0; dx < scale; dx++)
            for (int dy = 0; dy < scale; dy++)
                ssd1306_draw_char(buffer, x + dx, y + dy, *text); // Desenha cada caractere
        x += 6 * scale;  
        text++;
    }
}

// Função para exibir o menu no console
void print_menu() {  
    if (in_submenu) {  
        printf("\n%s:\n", menu_titles[current_menu]);  
        for (int i = 0; i < 5; i++)  
            if (menus[current_menu][i][0] != '\0')  
                printf("%s %s\n", (i == menu_index) ? "->" : "  ", menus[current_menu][i]);  
    } else {  
        printf("------------------------------------\n");  
        for (int i = 0; i < 6; i++)  
            printf("%s %s\n", (i == current_menu) ? ">>" : "  ", menu_titles[i]);  
        printf("---------------------------------\n");  
    }  
}

// Função para gerar um tom no buzzer
void tone(unsigned int frequency, unsigned int duration) {  
    unsigned long period = 1000000 / frequency, end_time = time_us_64() + (duration * 1000);  
    while (time_us_64() < end_time) {  
        gpio_put(BUZZER_PIN, 1);  // Liga o buzzer  
        sleep_us(period / 2);  
        gpio_put(BUZZER_PIN, 0);  // Desliga o buzzer  
        sleep_us(period / 2);  
    }  
}

// Função para atualizar os LEDs com base no menu selecionado
void update_leds() {  
    if ((current_menu == 1 && menu_index <= 5) || (current_menu == 2 && menu_index <= 5)) { 
        gpio_put(LED_RED, true);   // Ativa LED vermelho
        gpio_put(LED_GREEN, false); // Desativa LED verde
        gpio_put(LED_BLUE, false);  // Desativa LED azul
    } else if ((current_menu == 3 && menu_index <= 5) || (current_menu == 4 && menu_index <= 5) || (current_menu == 5 && menu_index <= 23)) {          
        gpio_put(LED_GREEN, true);  // Ativa LED verde
        gpio_put(LED_RED, false);    // Desativa LED vermelho
        gpio_put(LED_BLUE, false);   // Desativa LED azul
    } else {  
        gpio_put(LED_RED, false);    // Desativa LED vermelho
        gpio_put(LED_GREEN, false);   // Desativa LED verde
        gpio_put(LED_BLUE, true);     // Ativa LED azul
    }  
}

// Função para converter o valor lido do ADC para temperatura em graus Celsius  
float ler_adc_para_temperatura(uint16_t valor_adc) {  
    const float fator_conversao = 3.3f / (1 << 12);  // Conversão de 12 bits (0-4095) para 0-3.3V  
    float tensao = valor_adc * fator_conversao;      // Converte o valor ADC para tensão  
    return 27.0f - (tensao - 0.706f) / 0.001721f;  // Converte a tensão para temperatura em Celsius  
}  

int main() {
    stdio_init_all();  // Inicializa a biblioteca padrão de entrada/saída
    print_menu();  // Exibe o menu inicial
    adc_init();  // Inicializa o ADC (Conversor Analógico-Digital)
    adc_gpio_init(VRX_PIN);  // Inicializa o pino do eixo X do joystick
    adc_gpio_init(VRY_PIN);  // Inicializa o pino do eixo Y do joystick
    adc_set_temp_sensor_enabled(true);  // Habilita o sensor de temperatura
    adc_select_input(CANAL_ADC_TEMPERATURA);  // Seleciona o canal do ADC para temperatura
    gpio_init(SW_PIN);  
    gpio_set_dir(SW_PIN, GPIO_IN);  
    gpio_pull_up(SW_PIN);  // Ativa o pull-up no pino do switch
    gpio_init(BTN_A); gpio_set_dir(BTN_A, GPIO_IN); gpio_pull_up(BTN_A);  // Inicializa o botão A
    gpio_init(BTN_B); gpio_set_dir(BTN_B, GPIO_IN); gpio_pull_up(BTN_B);  // Inicializa o botão B
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);  // Inicializa o barramento I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configura o pino SDA para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Configura o pino SCL para I2C
    gpio_pull_up(I2C_SDA);  // Ativa o pull-up no pino SDA
    gpio_pull_up(I2C_SCL);  // Ativa o pull-up no pino SCL
    ssd1306_init();  // Inicializa o display OLED SSD1306
    
    // Define a área de renderização do display
    struct render_area frame_area = {.start_column = 0, .end_column = ssd1306_width - 1, .start_page = 0, .end_page = ssd1306_n_pages - 1};
    
    uint16_t valor_adc = adc_read();  // Lê o valor do ADC
    float temperatura_celsius = ler_adc_para_temperatura(valor_adc);  // Converte ADC para Celsius    
    calculate_render_area_buffer_length(&frame_area);  // Calcula o tamanho do buffer para a área de renderização
    
    uint8_t ssd[ssd1306_buffer_length];  // Buffer para o display
    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer
    render_on_display(ssd, &frame_area);  // Renderiza o buffer no display
    ssd1306_draw_string_scaled(ssd, 35, 10, "TalkGo", 2);   // Exibe o título "SpeakNow"
    ssd1306_draw_string_scaled(ssd, 50, 30, "SLZ MA", 1);   // Exibe o subtítulo "SLZ MA"
    
    char buffer[16]; 
    snprintf(buffer, sizeof(buffer), "%.fC", temperatura_celsius);  // Formata a temperatura com 2 casas decimais
    ssd1306_draw_string_scaled(ssd, 50, 50, buffer, 2);  // Exibe a temperatura no display
    render_on_display(ssd, &frame_area);  // Renderiza no display

    // Inicializa os LEDs
    gpio_init(LED_RED);  
    gpio_init(LED_GREEN);  
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);  
    gpio_set_dir(LED_GREEN, GPIO_OUT);  
    gpio_set_dir(LED_BLUE, GPIO_OUT);  
    gpio_put(LED_RED, false);   // Apaga LED vermelho
    gpio_put(LED_GREEN, false); // Apaga LED verde
    gpio_put(LED_BLUE, false);  // Apaga LED azul

    // Inicializa o buzzer
    gpio_init(BUZZER_PIN);  
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);  
    gpio_put(BUZZER_PIN, 0);  // Desliga buzzer

    uint16_t last_vrx_value = 2048, last_vry_value = 2048;  // Valores iniciais do joystick
    bool last_sw_state = false;  // Último estado do botão switch

    while (true) {
        adc_select_input(0);  // Seleciona o canal do joystick X
        uint16_t vrx_value = adc_read();  // Lê o valor do eixo X
        adc_select_input(1);  // Seleciona o canal do joystick Y
        uint16_t vry_value = adc_read();  // Lê o valor do eixo Y
        bool sw_pressed = !gpio_get(SW_PIN);  // Verifica se o botão switch foi pressionado

        // Verifica se o botão B foi pressionado
        if (gpio_get(BTN_B) == 0) {
            sleep_ms(200);  // Debounce para evitar múltiplas leituras erradas
            if (gpio_get(BTN_B) == 0) {  // Confirma a segunda leitura do botão
                for (int i = 0; i < 10; i++) {  // Toca um som de alerta
                    gpio_put(LED_GREEN, false); 
                    gpio_put(LED_BLUE, false);  
                    gpio_put(LED_RED, true);     
                    tone(600, 200); //acionando o buzzer             
                    sleep_ms(200);               
                    gpio_put(LED_RED, false);    
                    tone(800, 200);              
                    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                    render_on_display(ssd, &frame_area);
                    ssd1306_draw_string_scaled(ssd, 20, 30, "SOCORRO", 2);  // Exibe mensagem de socorro
                    render_on_display(ssd, &frame_area);
                }
                calculate_render_area_buffer_length(&frame_area);  // Recalcula a área de renderização
                memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer
                render_on_display(ssd, &frame_area);
                ssd1306_draw_string_scaled(ssd, 35, 10, "TalkGo", 2);   
                ssd1306_draw_string_scaled(ssd, 50, 30, "SLZ MA", 1);   
                snprintf(buffer, sizeof(buffer), "%.fC", temperatura_celsius); 
                ssd1306_draw_string_scaled(ssd, 50, 50, buffer, 2); 
                render_on_display(ssd, &frame_area);
                in_submenu = false; 
                menu_index = 0; 
                current_menu = 0; 
                selecting_option = false; 
                menu_updated = true; 
            }
        }

        // Controle de navegação no menu
        if (!in_submenu) {
            if (vrx_value > 1000 && last_vrx_value <= 1000) {  // Avança no menu
                current_menu = (current_menu + 1) % 6; 
                menu_updated = true;
                tone(500, 10); 
                memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                render_on_display(ssd, &frame_area);
                ssd1306_draw_string_scaled(ssd, 0, 30, menu_titles[current_menu], 2);  // Exibe o título do menu
                render_on_display(ssd, &frame_area);
            } else if (vrx_value < 3000 && last_vrx_value >= 3000) {  // Retrocede no menu
                current_menu = (current_menu - 1 + 6) % 6; 
                menu_updated = true;
                tone(500, 10); 
                memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                render_on_display(ssd, &frame_area);
                ssd1306_draw_string_scaled(ssd, 0, 30, menu_titles[current_menu], 2);  // Exibe o título do menu
                render_on_display(ssd, &frame_area);
            }

            // Se o botão do joystick foi pressionado
            if (sw_pressed && !last_sw_state) { 
                in_submenu = true; // Entra no submenu
                menu_index = 0; // Reseta a seleção do submenu
                menu_updated = true;
                sleep_ms(200);
                memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                render_on_display(ssd, &frame_area);
                ssd1306_draw_string_scaled(ssd, 0, 30, menus[current_menu][menu_index], 2);  // Exibe a opção do submenu
                render_on_display(ssd, &frame_area);
            }
        } else {
            // Navegação no submenu
            if (current_menu == 5) {  // Se estamos no menu de letras
                if (vrx_value > 1000 && last_vrx_value <= 1000) {  // Avança na seleção do alfabeto
                    menu_index = (menu_index + 1) % ALPHABET_SIZE; 
                    menu_updated = true;
                    tone(500, 10); 
                    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                    render_on_display(ssd, &frame_area);
                    ssd1306_draw_string_scaled(ssd, 50, 30, alphabet[menu_index], 2);  // Exibe a letra selecionada
                    render_on_display(ssd, &frame_area);
                } else if (vrx_value < 3000 && last_vrx_value >= 3000) {  // Retrocede na seleção do alfabeto
                    menu_index = (menu_index - 1 + ALPHABET_SIZE) % ALPHABET_SIZE; 
                    menu_updated = true;
                    tone(500, 10); 
                    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                    render_on_display(ssd, &frame_area);
                    ssd1306_draw_string_scaled(ssd, 50, 30, alphabet[menu_index], 2);  // Exibe a letra selecionada
                    render_on_display(ssd, &frame_area);
                }
                if (sw_pressed && !last_sw_state) {  // Se o botão do joystick foi pressionado
                    if (word_index < sizeof(word) - 1) {
                        word[word_index++] = 'A' + menu_index;  // Adiciona a letra correspondente
                        word[word_index] = '\0';  // Adiciona o terminador de string
                        printf("Palavra atual: %s\n", word);  // Exibe a palavra no console
                        memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                        render_on_display(ssd, &frame_area);
                        ssd1306_draw_string_scaled(ssd, 0, 30, word, 2);  // Exibe a palavra montada
                        render_on_display(ssd, &frame_area);
                    }
                }
            } else {
                // Navegação no submenu existente
                if (!selecting_option) {
                    if (vrx_value > 1000 && last_vrx_value <= 1000) {  // Avança na seleção do submenu
                        menu_index = (menu_index + 1) % 5; 
                        menu_updated = true;
                        tone(500, 10); 
                        memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                        render_on_display(ssd, &frame_area);
                        ssd1306_draw_string_scaled(ssd, 0, 30, menus[current_menu][menu_index], 2);  // Exibe a opção selecionada
                        render_on_display(ssd, &frame_area);
                    } else if (vrx_value < 3000 && last_vrx_value >= 3000) {  // Retrocede na seleção do submenu
                        menu_index = (menu_index - 1 + 5) % 5; 
                        menu_updated = true;
                        tone(500, 10); 
                        memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                        render_on_display(ssd, &frame_area);
                        ssd1306_draw_string_scaled(ssd, 0, 30, menus[current_menu][menu_index], 2);  // Exibe a opção selecionada
                        render_on_display(ssd, &frame_area);
                    }
                }

                // Se o botão do joystick foi pressionado
                if (sw_pressed && !last_sw_state) { 
                    if (selecting_option) {
                        printf("\nSelecionado: %s\n", menus[current_menu][menu_index]);
                        tone(current_menu == 1 ? 1000 : 400, current_menu == 1 ? 10000 : 500);  // Toca tom alto para emergência
                        sleep_ms(500);
                    } else if (current_menu == 1) {  // Se a opção de ajuda foi selecionada
                        selecting_option = true; // Ativa a seleção da opção
                        printf("\n%s\n", menus[current_menu][menu_index]);
                        memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                        render_on_display(ssd, &frame_area);
                        ssd1306_draw_string_scaled(ssd, 0, 30, menus[current_menu][menu_index], 2);  // Exibe a opção selecionada
                        render_on_display(ssd, &frame_area);
                        for (int i = 0; i < 5; i++) {  // Toque de sirene alternando entre duas frequências
                            tone(400, 300); 
                            gpio_put(LED_RED, true);
                            sleep_ms(200);
                            gpio_put(LED_RED, false);
                            tone(600, 300); 
                            sleep_ms(200);
                            gpio_put(LED_RED, true);
                        }
                        sleep_ms(500);
                    } else {
                        memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                        render_on_display(ssd, &frame_area);
                        ssd1306_draw_string_scaled(ssd, 0, 30, menus[current_menu][menu_index], 2);  // Exibe a opção selecionada
                        render_on_display(ssd, &frame_area);
                        selecting_option = true; // Ativa a seleção da opção
                        printf("\n%s\n", menus[current_menu][menu_index]);
                        for (int i = 0; i < 2; i++) {  // Toca uma sequência de tons
                            tone(200, 100);
                            sleep_ms(300);
                            tone(400, 100);
                            sleep_ms(300);
                            tone(100, 100);
                            sleep_ms(300);
                        }
                    }
                    sleep_ms(5000); // Aguarda antes de continuar
                }

                // Voltar ao menu principal
                if (selecting_option && sw_pressed && !last_sw_state) { 
                    printf("\nVoltando ao menu principal!\n");
                    in_submenu = false; // Retorna ao menu principal
                    menu_index = 0; // Reseta a seleção do submenu
                    current_menu = 0; // Retorna ao menu inicial
                    selecting_option = false; // Desativa a seleção da opção
                    menu_updated = true; // Indica que o menu foi atualizado
                    sleep_ms(500);
                    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
                    render_on_display(ssd, &frame_area);
                    ssd1306_draw_string_scaled(ssd, 35, 10, "TalkGo", 2);   
                    ssd1306_draw_string_scaled(ssd, 50, 30, "SLZ MA", 1);   
                    snprintf(buffer, sizeof(buffer), "%.fC", temperatura_celsius); 
                    ssd1306_draw_string_scaled(ssd, 50, 50, buffer, 2); 
                    render_on_display(ssd, &frame_area);
                }
            }
        }
        // Verifica se o botão A foi pressionado
        if (gpio_get(BTN_A) == 0) {
            printf("Palavra finalizada: %s\n", word); // Exibe a palavra finalizada no console
            memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
            render_on_display(ssd, &frame_area);
            ssd1306_draw_string_scaled(ssd, 20, 20, "Palavra", 2);   
            ssd1306_draw_string_scaled(ssd, 20, 50, word, 2);   
            render_on_display(ssd, &frame_area);
            for (int i = 0; i < 1; i++) {  // Toca uma sequência de tons
                tone(200, 100);
                sleep_ms(300);
                tone(400, 100);
                sleep_ms(300);
                tone(600, 100);
                sleep_ms(300);
            }
            sleep_ms(5000); // Aguarda antes de continuar
            memset(word, 0, sizeof(word)); // Limpa a palavra
            word_index = 0; // Reseta o índice da palavra
            in_submenu = false; // Retorna ao menu principal
            current_menu = 0; // Retorna ao menu inicial
            menu_index = 0; // Reseta a seleção do submenu
            menu_updated = true; // Indica que o menu foi atualizado
            sleep_ms(500);
            memset(ssd, 0, ssd1306_buffer_length);  // Limpa o display
            render_on_display(ssd, &frame_area);
            ssd1306_draw_string_scaled(ssd, 35, 10, "TalkGo", 2);   
            ssd1306_draw_string_scaled(ssd, 50, 30, "SLZ MA", 1);   
            snprintf(buffer, sizeof(buffer), "%.fC", temperatura_celsius); 
            ssd1306_draw_string_scaled(ssd, 50, 50, buffer, 2); 
            render_on_display(ssd, &frame_area);
        }
        update_leds(); // Atualiza os LEDs conforme necessário
        if (menu_updated) {
            print_menu(); // Exibe o menu atualizado
            menu_updated = false; // Reseta a flag de atualização
        }
        last_vrx_value = vrx_value; // Atualiza o valor anterior do joystick X
        last_vry_value = vry_value; // Atualiza o valor anterior do joystick Y
        last_sw_state = sw_pressed; // Atualiza o último estado do botão switch
        sleep_ms(100); // Delay para evitar processamento excessivo
    }
    return 0; // Finaliza o programa
}