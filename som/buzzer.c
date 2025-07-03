#include "buzzer.h"
#include "pico/stdlib.h"

static uint buzzer_pin;

void buzzer_init(uint pin) {
    buzzer_pin = pin;
    gpio_init(buzzer_pin);
    gpio_set_dir(buzzer_pin, GPIO_OUT);
    gpio_put(buzzer_pin, 0);
}

void tone(unsigned int frequency, unsigned int duration_ms) {
    if (frequency == 0) return;

    unsigned int period_us = 1000000 / frequency;
    unsigned int cycles = (duration_ms * 1000) / period_us;

    for (uint i = 0; i < cycles; i++) {
        gpio_put(buzzer_pin, 1);
        busy_wait_us_32(period_us / 2);
        gpio_put(buzzer_pin, 0);
        busy_wait_us_32(period_us / 2);
    }
}

void play_tone_sequence(const unsigned int *frequencies, const unsigned int *durations, size_t count, unsigned int pause_ms) {
    for (size_t i = 0; i < count; i++) {
        tone(frequencies[i], durations[i]);
        sleep_ms(pause_ms);
    }
}

// 🔊 Suavizado: Alerta de SOS com tons medianos
void play_sos_alert(void) {
    const unsigned int freqs[] = {400, 500};
    const unsigned int dur[] = {150, 150};
    for (int i = 0; i < 4; i++) {
        play_tone_sequence(freqs, dur, 2, 150);
    }
}

// 🔔 Suavizado: tom de seleção de emergência
void play_emergency_select_pattern(void) {
    const unsigned int freqs[] = {350, 450};
    const unsigned int dur[] = {180, 180};
    for (int i = 0; i < 2; i++) {
        play_tone_sequence(freqs, dur, 2, 120);
    }
}

// ✅ Suavizado: seleção comum
void play_normal_select_pattern(void) {
    const unsigned int freqs[] = {300, 400, 350};
    const unsigned int dur[] = {100, 100, 100};
    play_tone_sequence(freqs, dur, 3, 200);
}

// ✍️ Suavizado: finalização de palavra
void play_word_end_pattern(void) {
    const unsigned int freqs[] = {320, 380, 420};
    const unsigned int dur[] = {100, 100, 100};
    play_tone_sequence(freqs, dur, 3, 180);
}
